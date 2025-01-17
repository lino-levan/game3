#include "Log.h"
#include "types/Position.h"
#include "graphics/Tileset.h"
#include "entity/Player.h"
#include "game/Inventory.h"
#include "item/Seed.h"
#include "realm/Realm.h"

namespace Game3 {
	Seed::Seed(ItemID id_, std::string name_, Identifier crop_tilename, MoneyCount base_price, ItemCount max_count):
		Plantable(id_, std::move(name_), base_price, max_count), cropTilename(std::move(crop_tilename)) {}

	bool Seed::use(Slot slot, ItemStack &stack, const Place &place, Modifiers, std::pair<float, float>) {
		auto &realm = *place.realm;
		auto &tileset = realm.getTileset();

		if (auto tile = realm.tryTile(Layer::Terrain, place.position); tile && tileset.isInCategory(*tile, "base:category/farmland"_id)) {
			if (auto submerged = realm.tryTile(Layer::Submerged, place.position); !submerged || *submerged == tileset.getEmptyID()) {
				const InventoryPtr inventory = place.player->getInventory(0);
				auto inventory_lock = inventory->uniqueLock();
				return plant(inventory, slot, stack, place);
			}
		}

		return false;
	}

	bool Seed::drag(Slot slot, ItemStack &stack, const Place &place, Modifiers modifiers) {
		return use(slot, stack, place, modifiers, {0.f, 0.f});
	}

	bool Seed::plant(InventoryPtr inventory, Slot slot, ItemStack &stack, const Place &place) {
		if (stack.count == 0) {
			inventory->erase(slot);
			inventory->notifyOwner();
			return false;
		}

		place.set(Layer::Submerged, cropTilename);

		if (--stack.count == 0)
			inventory->erase(slot);
		inventory->notifyOwner();

		return true;
	}
}
