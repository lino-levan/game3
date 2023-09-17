#include <unordered_set>

#include "Tileset.h"
#include "biome/Grassland.h"
#include "entity/Chicken.h"
#include "entity/Dog.h"
#include "entity/Pig.h"
#include "entity/Sheep.h"
#include "game/Game.h"
#include "item/Item.h"
#include "lib/noise.h"
#include "realm/Realm.h"
#include "tileentity/ItemSpawner.h"
#include "tileentity/Tree.h"
#include "util/Timer.h"
#include "util/Util.h"
#include "worldgen/WorldGen.h"

namespace Game3 {
	static const std::vector<Identifier> grasses {
		"base:tile/grass_alt1"_id, "base:tile/grass_alt2"_id,
		"base:tile/grass"_id, "base:tile/grass"_id, "base:tile/grass"_id, "base:tile/grass"_id,
	};

	static const std::unordered_set<Identifier> grassSet {grasses.begin(), grasses.end()};

	void Grassland::init(Realm &realm, int noise_seed) {
		Biome::init(realm, noise_seed);
		forestPerlin = std::make_shared<noise::module::Perlin>();
		forestPerlin->SetSeed(-noise_seed * 3);
	}

	double Grassland::generate(Index row, Index column, std::default_random_engine &rng, const noise::module::Perlin &perlin, const WorldGenParams &params) {
		Realm &realm = *getRealm();
		const auto wetness    = params.wetness;
		const auto stoneLevel = params.stoneLevel;
		auto &tileset = realm.getTileset();
		const double noise = perlin.GetValue(row / Biome::NOISE_ZOOM, column / Biome::NOISE_ZOOM, 0.666);

		static const Identifier deeper_water  = "base:tile/deeper_water"_id;
		static const Identifier deep_water    = "base:tile/deep_water"_id;
		static const Identifier water         = "base:tile/water"_id;
		static const Identifier shallow_water = "base:tile/shallow_water"_id;
		static const Identifier sand          = "base:tile/sand"_id;
		static const Identifier light_grass   = "base:tile/light_grass"_id;
		static const Identifier stone         = "base:tile/stone"_id;
		static const Identifier forest_floor  = "base:tile/forest_floor"_id;
		static const Identifier water_fluid   = "base:fluid/water"_id;

		if (noise < wetness + 0.3) {
			realm.setTile(Layer::Terrain, {row, column}, sand, false, true);
			realm.setFluid({row, column}, water_fluid, FluidTile::INFINITE, false, true);
		} else if (noise < wetness + 0.4) {
			realm.setTile(Layer::Terrain, {row, column}, sand, false, true);
		} else if (noise < wetness + 0.5) {
			realm.setTile(Layer::Terrain, {row, column}, light_grass, false, true);
		} else if (stoneLevel < noise) {
			realm.setTile(Layer::Terrain, {row, column}, stone, false, true);
		} else {
			if (std::uniform_int_distribution(0, 15)(rng) == 0)
				realm.setTile(Layer::Terrain, {row, column}, choose(tileset.getTilesByCategory("base:category/small_flowers"), rng), false, true);
			else
				realm.setTile(Layer::Terrain, {row, column}, choose(grasses, rng), false, true);
			const double forest_noise = forestPerlin->GetValue(row / Biome::NOISE_ZOOM, column / Biome::NOISE_ZOOM, 0.5);
			if (params.forestThreshold < forest_noise) {
				std::default_random_engine tree_rng(static_cast<uint_fast32_t>(forest_noise * 1'000'000'000.));
				if ((abs(row) % 2) == (std::uniform_int_distribution(0, 39)(tree_rng) < 20)) {
					// realm.add(TileEntity::create<Tree>(realm.getGame(), choose(trees, rng), "base:tile/tree0"_id, Position(row, column), Tree::MATURITY));
					static const std::vector<Identifier> trees {
						"base:tile/tree1"_id,
						"base:tile/tree2"_id,
						"base:tile/tree3"_id,
						"base:tile/tree1_empty"_id,
						"base:tile/tree2_empty"_id,
						"base:tile/tree3_empty"_id
					};
					realm.setTile(Layer::Submerged, {row, column}, choose(trees, rng), false, true);
				}
				realm.setTile(Layer::Terrain, {row, column}, forest_floor, false, true);
			}
		}

		return noise;
	}

	void Grassland::postgen(Index row, Index column, std::default_random_engine &rng, const noise::module::Perlin &perlin, const WorldGenParams &params) {
		Realm &realm = *getRealm();
		constexpr double factor = 10;
		static std::uniform_int_distribution distribution(0, 99);

		// TODO!: cropification
		if (params.antiforestThreshold > perlin.GetValue(row / Biome::NOISE_ZOOM * factor, column / Biome::NOISE_ZOOM * factor, 0.))
			if (auto tile_entity = realm.tileEntityAt({row, column}); tile_entity && tile_entity->is("base:te/tree"_id) && !std::dynamic_pointer_cast<Tree>(tile_entity)->hasHive())
				realm.removeSafe(tile_entity);

		const auto &tileset = realm.getTileset();
		const auto tile1 = tileset[realm.getTile(Layer::Terrain, {row, column})];

		if (grassSet.contains(tile1) && realm.middleEmpty({row, column})) {
			if (distribution(rng) < 2)
				realm.setTile(Layer::Submerged, {row, column}, choose(tileset.getCategoryIDs("base:category/flowers"), rng), false, true);

			std::shared_ptr<Animal> animal;

			switch (std::uniform_int_distribution(1, 600)(rng)) {
				case 1:
				case 2:
					animal = realm.spawn<Sheep>({row, column});
					break;
				case 3:
				case 4:
					animal = realm.spawn<Pig>({row, column});
					break;
				case 5:
				case 6:
					animal = realm.spawn<Chicken>({row, column});
					break;
				case 7:
					animal = realm.spawn<Dog>({row, column});
					break;
			}

			if (animal)
				animal->direction = randomDirection();
		}
	}
}
