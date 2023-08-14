#include "Log.h"
#include "entity/ClientPlayer.h"
#include "game/ClientGame.h"
#include "packet/EntityMovedPacket.h"

namespace Game3 {
	EntityMovedPacket::EntityMovedPacket(const Entity &entity):
		EntityMovedPacket(Args{entity.globalID, entity.nextRealm == -1? entity.realmID : entity.nextRealm, entity.getPosition(), entity.direction, entity.offset, entity.zSpeed, true}) {}

	void EntityMovedPacket::encode(Game &, Buffer &buffer) const {
		buffer << arguments.globalID << arguments.realmID << arguments.position << arguments.facing << arguments.offset << arguments.zSpeed << arguments.adjustOffset;
	}

	void EntityMovedPacket::decode(Game &, Buffer &buffer) {
		buffer >> arguments.globalID >> arguments.realmID >> arguments.position >> arguments.facing >> arguments.offset >> arguments.zSpeed >> arguments.adjustOffset;
	}

	void EntityMovedPacket::handle(ClientGame &game) {
		RealmPtr realm = game.tryRealm(arguments.realmID);
		if (!realm) {
			WARN("Couldn't find realm " << arguments.realmID << " in EntityMovedPacket.");
			return;
		}

		EntityPtr entity = game.getAgent<Entity>(arguments.globalID);
		if (!entity) {
			WARN("Couldn't find entity " << arguments.globalID << ". Player is " << game.player->getGID());
			return;
		}

		if (!entity->isPlayer()) {
			auto &entity_ref = *entity;
			INFO("Moving non-player entity " << arguments.globalID << " (" << typeid(entity_ref).name() << "). Player is " << game.player->getGID());
		}

		const double apparent_x = entity->offset.x + double(entity->getPosition().column);
		const double apparent_y = entity->offset.y + double(entity->getPosition().row);

		entity->direction = arguments.facing;
		entity->teleport(arguments.position, realm);

		if (arguments.adjustOffset) {
			entity->offset.x = apparent_x - entity->getPosition().column;
			entity->offset.y = apparent_y - entity->getPosition().row;
		} else if (arguments.offset)
			entity->offset = *arguments.offset;

		if (arguments.zSpeed)
			entity->zSpeed = *arguments.zSpeed;
	}
}
