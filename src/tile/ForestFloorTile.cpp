#include "Log.h"
#include "types/Position.h"
#include "entity/ItemEntity.h"
#include "entity/Player.h"
#include "game/Inventory.h"
#include "realm/Realm.h"
#include "threading/ThreadContext.h"
#include "tile/ForestFloorTile.h"
#include "util/Util.h"

namespace Game3 {
	ForestFloorTile::ForestFloorTile():
		Tile(ID()) {}

	bool ForestFloorTile::interact(const Place &place, Layer layer, ItemStack *, Hand) {
		if (layer != Layer::Terrain)
			return false;

		RealmPtr realm = place.realm;
		assert(realm);
		PlayerPtr player = place.player;
		assert(player);
		InventoryPtr inventory = player->getInventory(0);
		assert(inventory);

		auto lock = inventory->uniqueLock();

		if (player->hasTooldown())
			return false;

		if (ItemStack *active = inventory->getActive(); active && active->hasAttribute("base:attribute/shovel")) {
			player->setTooldown(1.f);

			if (active->reduceDurability())
				inventory->erase(inventory->activeSlot);

			Game &game = realm->getGame();
			player->give(ItemStack(game, "base:item/dirt"));
			if (std::uniform_int_distribution(1, 10)(threadContext.rng) <= 2)
				player->give(ItemStack(game, "base:item/moss"));

			inventory->notifyOwner();
			return true;
		}

		return false;
	}

	void ForestFloorTile::randomTick(const Place &place) {
		Tile::randomTick(place);

		Realm &realm = *place.realm;
		Game &game = realm.getGame();

		std::uniform_int_distribution distribution{0, 99};
		if (distribution(threadContext.rng) != 0)
			return;

		// If there are any adjacent or overlapping items, give up and don't spawn anything.
		if (auto entities = realm.getEntities(place.position.getChunk())) {
			auto lock = entities->sharedLock();
			for (const EntityPtr &entity: *entities)
				if (entity->position.taxiDistance(place.position) <= 3 && std::dynamic_pointer_cast<ItemEntity>(entity))
					return;
		}

		if (!place.isPathable())
			return;

		std::vector<ItemStack> mushrooms {
			{game, "base:item/saffron_milkcap"},
			{game, "base:item/saffron_milkcap"},
			{game, "base:item/saffron_milkcap"},
			{game, "base:item/honey_fungus"},
			{game, "base:item/honey_fungus"},
			{game, "base:item/brittlegill"},
		};

		choose(mushrooms, threadContext.rng).spawn(place.realm, place.position);
	}
}
