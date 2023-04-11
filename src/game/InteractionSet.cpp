#include "Position.h"
#include "Tileset.h"
#include "entity/Player.h"
#include "game/Game.h"
#include "game/InteractionSet.h"
#include "game/Inventory.h"
#include "realm/Realm.h"

namespace Game3 {
	bool StandardInteractions::interact(const Place &place) const {
		// TODO: handle other tilemaps

		const auto &position = place.position;
		auto &player = *place.player;
		auto &realm  = *place.realm;
		auto &game   = realm.getGame();
		auto &inventory = *player.inventory;

		const Index index = realm.getIndex(position);
		auto &tileset = realm.getTileset();
		auto &tilemap2 = realm.tilemap2;
		const auto &tile1 = tileset[place.getLayer1()];
		const auto &tile2 = tileset[place.getLayer2()];

		if (auto *active = inventory.getActive()) {
			if (active->item->canUseOnWorld() && active->item->use(inventory.activeSlot, *active, place))
				return true;


			if (active->has(ItemAttribute::Hammer)) {
				const TileID tile2 = tilemap2->tiles.at(index);
				ItemStack stack;
				if (tileset.getItemStack(game, tileset[tile2], stack) && !inventory.add(stack)) {
					if (active->reduceDurability())
						inventory.erase(inventory.activeSlot);
					realm.setLayer2(position, tileset.getEmpty());
					return true;
				}
			}

			if (active->has(ItemAttribute::Shovel)) {
				if (tile2 == "base:tile/ash"_id) {
					realm.setLayer2(position, "base:tile/empty"_id);
					player.give({game, "base:item/ash"_id, 1});
					realm.getGame().activateContext();
					realm.reupload();
					return true;
				}
			}
		}

		std::optional<Identifier> item;
		std::optional<ItemAttribute> attribute;

		if (tile1 == "base:tile/sand"_id) {
			item.emplace("base:item/sand"_id);
			attribute.emplace(ItemAttribute::Shovel);
		} else if (tile1 == "base:tile/shallow_water"_id) {
			item.emplace("base:item/clay"_id);
			attribute.emplace(ItemAttribute::Shovel);
		} else if (tile1 == "base:tile/volcanic_sand"_id) {
			item.emplace("base:item/volcanic_sand"_id);
			attribute.emplace(ItemAttribute::Shovel);
		} else if (tileset.isInCategory(tile1, "base:category/dirt")) {
			item.emplace("base:item/dirt"_id);
			attribute.emplace(ItemAttribute::Shovel);
		} else if (tile1 == "base:tile/stone"_id) {
			item.emplace("base:item/stone"_id);
			attribute.emplace(ItemAttribute::Pickaxe);
		}

		if (item && attribute && !player.hasTooldown()) {
			if (auto *stack = inventory.getActive()) {
				if (stack->has(*attribute) && !inventory.add({game, *item, 1})) {
					player.setTooldown(1.f);
					if (stack->reduceDurability())
						inventory.erase(inventory.activeSlot);
					else
						// setTooldown doesn't call notifyOwner on the player's inventory, so we have to do it here.
						player.inventory->notifyOwner();
					return true;
				}
			}
		}

		return false;
	}

	bool StandardInteractions::damageGround(const Place &place) const {
		// TODO: handle other tilemaps

		const auto &tile3 = place.realm->getTileset()[place.getLayer3()];
		if (tile3 == "base:tile/charred_stump"_id) {
			place.realm->setLayer3(place.position, "base:tile/empty"_id);
			return true;
		}

		return false;
	}
}
