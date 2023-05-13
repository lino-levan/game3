#pragma once

#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

#include "Types.h"
#include "net/Buffer.h"
#include "net/Sock.h"
#include "util/Math.h"

namespace Game3 {
	class ClientGame;
	class Packet;

	class LocalClient: public std::enable_shared_from_this<LocalClient> {
		public:
			constexpr static size_t MAX_PACKET_SIZE = 1 << 24;

			std::weak_ptr<ClientGame> weakGame;

			LocalClient() = default;

			void connect(std::string_view hostname, uint16_t port);
			void read();
			void send(const Packet &);
			bool isConnected() const;
			std::shared_ptr<ClientGame> lockGame() const;
			void setToken(const std::string &hostname, const std::string &username, Token);
			void readTokens(const std::filesystem::path &);
			void saveTokens() const;
			void saveTokens(const std::filesystem::path &);
			const std::string & getHostname() const;

		private:
			enum class State {Begin, Data};
			State state = State::Begin;
			Buffer buffer;
			uint16_t packetType = 0;
			uint32_t payloadSize = 0;
			std::shared_ptr<Sock> sock;
			std::vector<uint8_t> headerBytes;
			std::map<std::string, std::map<std::string, Token>> tokenDatabase;
			std::optional<std::filesystem::path> tokenDatabasePath;

			template <std::integral T>
			void sendRaw(T value) {
				T little = toLittle(value);
				sock->send(&little, sizeof(little));
			}
	};
}
