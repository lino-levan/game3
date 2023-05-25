#include "Log.h"
#include "game/ClientGame.h"
#include "packet/EntitySetPathPacket.h"

namespace Game3 {
	EntitySetPathPacket::EntitySetPathPacket(Entity &entity):
		EntitySetPathPacket(entity.globalID, entity.realmID, entity.getPosition(), entity.copyPath<std::vector>()) {}

	void EntitySetPathPacket::handle(ClientGame &game) {
		auto iter = game.realms.find(realmID);
		if (iter == game.realms.end()) {
			ERROR("EntitySetPathPacket: can't find realm " << realmID);
			return;
		}

		auto realm = iter->second;
		auto entity = realm->getEntity(globalID);
		if (!entity) {
			ERROR("EntitySetPathPacket: can't find entity " << globalID << " in realm " << realmID);
			return;
		}

		entity->path = {path.begin(), path.end()};

		const auto &entity_ref = *entity;
		SUCCESS("Set path of " << typeid(entity_ref).name() << " " << globalID << " in realm " << realmID << "; new size: " << entity->path.size());
	}
}