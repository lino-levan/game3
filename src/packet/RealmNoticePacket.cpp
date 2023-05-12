#include "game/ClientGame.h"
#include "packet/RealmNoticePacket.h"
#include "realm/Realm.h"

namespace Game3 {
	void RealmNoticePacket::handle(ClientGame &game) const {
		if (!game.realms.contains(realmID))
			game.realms.emplace(realmID, Realm::create(game, realmID, type, tileset, seed));
	}
}