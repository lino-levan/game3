#include "game/ServerGame.h"
#include "graphics/Tileset.h"
#include "net/Server.h"
#include "net/RemoteClient.h"
#include "packet/ErrorPacket.h"
#include "packet/TilesetRequestPacket.h"
#include "packet/TilesetResponsePacket.h"

namespace Game3 {
	void TilesetRequestPacket::handle(ServerGame &game, RemoteClient &client) {
		RealmPtr realm = game.getRealm(realmID);
		if (!realm) {
			client.send(ErrorPacket("Invalid realm ID"));
			return;
		}

		Tileset &tileset = realm->getTileset();
		const auto &names = tileset.getNames();
		client.send(TilesetResponsePacket(realmID, {names.begin(), names.end()}));
	}
}
