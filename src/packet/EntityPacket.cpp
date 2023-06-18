#include "Log.h"
#include "game/ClientGame.h"
#include "game/Game.h"
#include "net/Buffer.h"
#include "packet/EntityPacket.h"
#include "packet/PacketError.h"
#include "entity/Entity.h"
#include "entity/EntityFactory.h"

namespace Game3 {
	EntityPacket::EntityPacket(EntityPtr entity_):
		entity(std::move(entity_)),
		identifier(entity->type),
		globalID(entity->globalID),
		realmID(entity->realmID) {}

	void EntityPacket::decode(Game &game, Buffer &buffer) {
		buffer >> globalID >> identifier >> realmID;
		auto realm_iter = game.realms.find(realmID);
		if (realm_iter == game.realms.end())
			throw PacketError("Couldn't find realm " + std::to_string(realmID) + " in EntityPacket");
		auto realm = realm_iter->second;
		if (auto found = realm->getEntity(globalID)) {
			wasFound = true;
			(entity = found)->decode(buffer);
		} else {
			wasFound = false;
			auto factory = game.registry<EntityFactoryRegistry>()[identifier];
			entity = (*factory)(game);
			entity->type = identifier;
			entity->init(game);
			entity->setGID(globalID);
			entity->decode(buffer);
		}
	}

	void EntityPacket::encode(Game &, Buffer &buffer) const {
		assert(entity);
		buffer << globalID << identifier << realmID;
		entity->encode(buffer);
	}

	void EntityPacket::handle(ClientGame &game) {
		if (wasFound)
			return;
		auto iter = game.realms.find(realmID);
		if (iter == game.realms.end())
			throw PacketError("Couldn't find realm " + std::to_string(realmID) + " in EntityPacket");
		iter->second->add(entity, entity->getPosition());
	}
}
