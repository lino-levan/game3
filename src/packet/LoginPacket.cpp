#include "Log.h"
#include "graphics/Tileset.h"
#include "entity/ServerPlayer.h"
#include "game/ServerGame.h"
#include "net/Server.h"
#include "net/RemoteClient.h"
#include "packet/LoginPacket.h"
#include "packet/LoginStatusPacket.h"
#include "packet/RealmNoticePacket.h"
#include "packet/PacketError.h"

namespace Game3 {
	void LoginPacket::handle(ServerGame &game, RemoteClient &client) {
		if (!client.getPlayer()) {
			auto server = game.getServer();
			std::string display_name;
			nlohmann::json json;
			std::optional<Place> release_place;

			if (!game.hasPlayer(username) && server->generateToken(username) == token && game.database.readUser(username, &display_name, &json, &release_place)) {
				auto player = ServerPlayer::fromJSON(game, json);
				player->username = username;
				client.setPlayer(player);
				game.addPlayer(player);
				RealmPtr realm = game.getRealm(player->realmID);
				player->setRealm(realm);
				player->weakClient = std::static_pointer_cast<RemoteClient>(client.shared_from_this());
				player->notifyOfRealm(*realm);
				SUCCESS("Player " << username << "'s GID is " << player->globalID);
				player->init(game);
				client.send(LoginStatusPacket(true, player->globalID, username, display_name, player));
				server->setupPlayer(client);
				realm->addPlayer(player);
				if (release_place) {
					player->teleport(release_place->position, release_place->realm, MovementContext{.isTeleport = true});
					game.database.writeReleasePlace(username, std::nullopt);
				}
				return;
			}
		}

		client.send(LoginStatusPacket(false));
	}
}
