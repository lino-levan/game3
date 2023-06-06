#include "Log.h"
#include "command/local/UsageCommand.h"
#include "net/LocalClient.h"
#include "packet/LoginPacket.h"
#include "util/Util.h"

namespace Game3 {
	void UsageCommand::operator()(LocalClient &client) {
		std::cerr.imbue(std::locale(""));
		INFO("");
		INFO("Header bytes: \e[3" << (client.headerBytes.empty()? '2' : '1') << 'm' << client.headerBytes.size() << "\e[39m");
		INFO("Payload size: \e[33m" << client.payloadSize << "\e[39m");
		{
			INFO("Packets received:");
			std::shared_lock lock(client.receivedPacketCountsMutex);
			for (const auto &[packet_id, count]: client.receivedPacketCounts)
				INFO("    " << std::setw(3) << std::right << packet_id << "\e[2m:\e[22m " << count);
		}
		{
			INFO("Packets sent:");
			std::shared_lock lock(client.sentPacketCountsMutex);
			for (const auto &[packet_id, count]: client.sentPacketCounts)
				INFO("    " << std::setw(3) << std::right << packet_id << "\e[2m:\e[22m " << count);
		}

		INFO("Bytes read: \e[36m" << client.bytesRead << "\e[39m");
		INFO("Bytes written: \e[35m" << client.bytesWritten << "\e[39m");
		std::cerr.imbue(std::locale("C"));
	}
}
