#include <cassert>
#include <csignal>

#include "Log.h"
#include "game/ServerGame.h"
#include "net/Buffer.h"
#include "net/Server.h"
#include "net/RemoteClient.h"
#include "packet/ChunkTilesPacket.h"
#include "packet/Packet.h"
#include "packet/PacketError.h"
#include "packet/PacketFactory.h"
#include "util/Math.h"
#include "util/Util.h"

namespace Game3 {
	void RemoteClient::handleInput(std::string_view string) {
		if (string.empty())
			return;

		std::stringstream ss;
		for (const uint8_t byte: string)
			ss << ' ' << std::hex << std::setfill('0') << std::setw(2) << std::right << static_cast<uint16_t>(byte) << std::dec;
		auto str = std::string(string);
		while (!str.empty() && (str.back() == '\r' || str.back() == '\n'))
			str.pop_back();

		headerBytes.insert(headerBytes.end(), string.begin(), string.end());

		if (state == State::Begin) {
			receiveBuffer.clear();
			packetType = 0;
			payloadSize = 0;

			if (6 <= headerBytes.size()) {
				packetType = headerBytes[0] | (static_cast<uint16_t>(headerBytes[1]) << 8);
				payloadSize = headerBytes[2] | (static_cast<uint32_t>(headerBytes[3]) << 8) | (static_cast<uint32_t>(headerBytes[4]) << 16) | (static_cast<uint32_t>(headerBytes[5]) << 24);
				headerBytes.erase(headerBytes.begin(), headerBytes.begin() + 6);
				state = State::Data;
			}
		}

		if (state == State::Data) {
			if (MAX_PACKET_SIZE < receiveBuffer.size() + headerBytes.size())
				throw PacketError("Packet too large");

			const size_t to_append = std::min(payloadSize - receiveBuffer.size(), headerBytes.size());

			receiveBuffer.append(headerBytes.begin(), headerBytes.begin() + to_append);
			if (to_append == headerBytes.size())
				headerBytes.clear();
			else
				headerBytes.erase(headerBytes.begin(), headerBytes.begin() + to_append);

			if (payloadSize < receiveBuffer.size())
				throw std::logic_error("Buffer grew too large");

			if (payloadSize == receiveBuffer.size()) {
				if (receiveBuffer.context.expired())
					receiveBuffer.context = server.game;

				auto packet = (*server.game->registry<PacketFactoryRegistry>()[packetType])();

				try {
					packet->decode(*server.game, receiveBuffer);
				} catch (const std::exception &err) {
					ERROR("Couldn't decode packet of type " << packetType << ", size " << payloadSize << ": " << err.what());
					server.close(*this);
					return;
				} catch (...) {
					ERROR("Couldn't decode packet of type " << packetType << ", size " << payloadSize);
					server.close(*this);
					return;
				}

				assert(receiveBuffer.empty());
				receiveBuffer.clear();
				server.game->queuePacket(shared_from_this(), packet);
				state = State::Begin;
			}
		}
	}

	bool RemoteClient::send(const Packet &packet) {
		if (!packet.valid) {
			WARN("Dropping invalid packet of type " << typeid(packet).name());
			return false;
		}

		assert(server.game);
		Buffer send_buffer;
		packet.encode(*server.game, send_buffer);
		assert(send_buffer.size() < UINT32_MAX);
		const auto size = toLittle(static_cast<uint32_t>(send_buffer.size()));
		const auto packet_id = toLittle(packet.getID());

		std::span span = send_buffer.getSpan();
		std::string to_send;
		to_send.reserve(span.size_bytes() + sizeof(packet_id) + sizeof(size));
		to_send.append(reinterpret_cast<const char *>(&packet_id), sizeof(packet_id));
		to_send.append(reinterpret_cast<const char *>(&size), sizeof(size));
		to_send.append(span.begin(), span.end());
		send(to_send);
		return true;
	}

	void RemoteClient::sendChunk(Realm &realm, ChunkPosition chunk_position, bool can_request, uint64_t counter_threshold) {
		assert(server.game);

		if (counter_threshold != 0 && realm.tileProvider.contains(chunk_position) && realm.tileProvider.getUpdateCounter(chunk_position) < counter_threshold)
			return;

		try {
			realm.sendToOne(*this, chunk_position);
		} catch (const std::out_of_range &) {
			if (!can_request)
				throw;
			realm.requestChunk(chunk_position, shared_from_this());
		}
	}

	template <typename T>
	requires (!std::derived_from<T, Packet>)
	void RemoteClient::send(const T &value) {
		server.send(*this, value);
	}

	void RemoteClient::startBuffering() {
		++sendBuffer;
	}

	void RemoteClient::flushBuffer(bool force) {
		if (!force && !sendBuffer.active())
			return;
		std::vector<char> moved_buffer;
		{
			auto buffer_lock = sendBuffer.uniqueLock();
			if (sendBuffer.bytes.empty())
				return;
			moved_buffer = std::move(sendBuffer.bytes);
		}
		std::unique_lock network_lock(networkMutex);
		server.send(*this, std::move(moved_buffer), true);
	}

	void RemoteClient::stopBuffering() {
		if (!(--sendBuffer).active())
			flushBuffer(true);
	}

	bool RemoteClient::isBuffering() const {
		return sendBuffer.active();
	}

	void RemoteClient::removeSelf() {
		INFO("Removing client from IP " << ip);

		if (server.game)
			if (ServerPlayerPtr player = getPlayer())
				server.game->queueRemoval(player);

		try {
			socket.shutdown();
		} catch (const std::system_error &) {
			// Sometimes the pipe is broken.
		}

		socket.next_layer().close();

		auto &clients = server.getClients();
		auto lock = clients.uniqueLock();
		clients.erase(shared_from_this());
	}
}
