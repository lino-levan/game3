#include "Tileset.h"
#include "entity/Blacksmith.h"
#include "game/Game.h"
#include "realm/Realm.h"
#include "tileentity/Building.h"
#include "tileentity/CraftingStation.h"
#include "util/Timer.h"
#include "util/Util.h"
#include "worldgen/BlacksmithGen.h"
#include "worldgen/Carpet.h"
#include "worldgen/Indoors.h"

namespace Game3::WorldGen {
	void generateBlacksmith(const std::shared_ptr<Realm> &realm, std::default_random_engine &rng, const std::shared_ptr<Realm> &parent_realm, Index width, Index height, const Position &entrance) {
		Timer timer("GenerateBlacksmith");

		generateIndoors(realm, rng, parent_realm, width, height, entrance);
		Game &game = realm->getGame();

		realm->setTile(2, {1, 3}, "base:tile/furnace"_id);
		realm->setTile(2, {1, 5}, "base:tile/anvil"_id);
		realm->add(TileEntity::create<CraftingStation>(game, "base:tile/furnace"_id, Position(1, 3), "base:station/furnace"_id));
		realm->add(TileEntity::create<CraftingStation>(game, "base:tile/anvil"_id,   Position(1, 5), "base:station/anvil"_id));
		realm->extraData["furnace"] = Position(1, 3);
		realm->extraData["anvil"]   = Position(1, 5);

		realm->setTile(2, {height / 2, width / 2 - 1}, "base:tile/counter_w"_id);
		realm->setTile(2, {height / 2, width / 2},     "base:tile/counter_we"_id);
		realm->setTile(2, {height / 2, width / 2 + 1}, "base:tile/counter_e"_id);
		realm->extraData["counter"] = Position(height / 2 - 1, width / 2);

		const auto &beds = realm->getTileset().getTilesByCategory("base:category/beds");
		std::array<Index, 2> edges {1, width - 2};
		const Position bed_position(1, choose(edges, rng));
		realm->setTile(2, bed_position, choose(beds, rng));
		realm->extraData["bed"] = bed_position;

		const Position building_position = entrance - Position(1, 0);
		realm->spawn<Blacksmith>(bed_position, parent_realm->id, realm->id, building_position, parent_realm->closestTileEntity<Building>(building_position,
			[](const auto &building) { return building->tileID == "base:tile/keep_sw"_id; }));
	}
}
