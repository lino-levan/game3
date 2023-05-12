#include <cctype>
#include <filesystem>
#include <fstream>

#include <event2/thread.h>

#include "Log.h"
#include "game/ServerGame.h"
#include "net/LocalServer.h"
#include "net/RemoteClient.h"
#include "net/Server.h"
#include "net/SSLServer.h"
#include "packet/ProtocolVersionPacket.h"
#include "realm/Overworld.h"
#include "util/Crypto.h"
#include "util/FS.h"
#include "util/Util.h"
#include "worldgen/Overworld.h"
#include "worldgen/WorldGen.h"

namespace Game3 {
	LocalServer::LocalServer(std::shared_ptr<Server> server_, std::string_view secret_):
	server(std::move(server_)), secret(secret_) {
		server->addClient = [this](auto &, int new_client, std::string_view ip) {
			auto game_client = std::make_shared<RemoteClient>(*this, new_client, ip);
			server->getClients().try_emplace(new_client, std::move(game_client));
			INFO("Adding " << new_client << " from " << ip);
		};

		server->closeHandler = [](int client_id) {
			INFO("Closing " << client_id);
		};

		server->messageHandler = [](GenericClient &generic_client, std::string_view message) {
			generic_client.handleInput(message);
		};
	}

	LocalServer::~LocalServer() {
		server->addClient = {};
		server->closeHandler = {};
		server->messageHandler = {};
	}

	void LocalServer::run() {
		server->run();
	}

	void LocalServer::stop() {
		server->stop();
	}

	void LocalServer::send(const GenericClient &client, std::string_view string) {
		send(client.id, string);
	}

	void LocalServer::send(int id, std::string_view string) {
		server->send(id, string);
	}

	static std::shared_ptr<Server> global_server;
	static bool running = true;

	int LocalServer::main(int, char **) {
		evthread_use_pthreads();

		std::string secret;

		if (std::filesystem::exists(".secret")) {
			secret = readFile(".secret");
		} else {
			secret = generateSecret(8);
			std::ofstream ofs(".secret");
			ofs << secret;
		}

		global_server = std::make_shared<SSLServer>(AF_INET6, "::0", 12255, "private.crt", "private.key", 2);

		if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
			throw std::runtime_error("Couldn't register SIGPIPE handler");

		if (signal(SIGINT, +[](int) { running = false; global_server->stop(); }) == SIG_ERR)
			throw std::runtime_error("Couldn't register SIGINT handler");

		auto game_server = std::make_shared<LocalServer>(global_server, secret);
		auto game = std::dynamic_pointer_cast<ServerGame>(Game::create(Side::Server, game_server));
		game->initEntities();

		constexpr size_t seed = 1621;
		auto realm = Realm::create<Overworld>(*game, 1, Overworld::ID(), "base:tileset/monomap"_id, seed);
		realm->outdoors = true;
		std::default_random_engine rng;
		rng.seed(seed);
		WorldGen::generateOverworld(realm, seed, {}, {{-1, -1}, {1, 1}}, true);
		game->realms.emplace(realm->id, realm);
		game->activeRealm = realm;
		game->initInteractionSets();

		std::thread tick_thread = std::thread([&] {
			while (running) {
				game->tick();
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		});

		game_server->run();
		tick_thread.join();

		return 0;
	}
}