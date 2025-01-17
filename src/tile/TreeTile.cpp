#include "types/Position.h"
#include "entity/Player.h"
#include "game/Crop.h"
#include "game/Inventory.h"
#include "threading/ThreadContext.h"
#include "tile/TreeTile.h"

namespace Game3 {
	TreeTile::TreeTile(std::shared_ptr<Crop> crop_):
		CropTile(ID(), std::move(crop_)) {}

	bool TreeTile::interact(const Place &place, Layer layer, ItemStack *used_item, Hand hand) {
		assert(!crop->stages.empty());

		PlayerPtr player = place.player;
		Game &game = player->getGame();

		const InventoryPtr inventory = player->getInventory(0);
		auto inventory_lock = inventory->uniqueLock();

		if (ItemStack *active_stack = inventory->getActive()) {
			if (active_stack->hasAttribute("base:attribute/axe")) {
				auto tilename = place.getName(layer);

				// Remove tree
				place.set(layer, 0);

				// Handle axe durability
				if (active_stack->reduceDurability())
					inventory->erase(inventory->activeSlot);

				// Make sure tree is fully grown before giving any products
				if (tilename && tilename->get() == crop->stages.back())
					for (const ItemStack &stack: crop->products.getStacks())
						if (std::optional<ItemStack> leftover = inventory->add(stack))
							leftover->spawn(place.realm, place.position);

				inventory->notifyOwner();
				return true;
			}
		}

		if (auto honey = crop->customData.find("honey"); honey != crop->customData.end()) {
			if (auto tilename = place.getName(layer); tilename && tilename->get() == honey->at("full").get<Identifier>()) {
				if (!inventory->add({game, honey->at("item").get<Identifier>()})) {
					place.set(layer, honey->at("empty").get<Identifier>());
					return true;
				}
			}
		}

		return Tile::interact(place, layer, used_item, hand);
	}
}
