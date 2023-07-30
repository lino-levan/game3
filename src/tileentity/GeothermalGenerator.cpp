#include <iostream>

#include "Tileset.h"
#include "game/ClientGame.h"
// #include "packet/OpenEnergyLevelPacket.h"
#include "realm/Realm.h"
#include "tileentity/GeothermalGenerator.h"
#include "ui/SpriteRenderer.h"

namespace Game3 {
	GeothermalGenerator::GeothermalGenerator(Identifier tile_id, Position position_):
		TileEntity(std::move(tile_id), ID(), position_, true) {}

	GeothermalGenerator::GeothermalGenerator(Position position_):
		GeothermalGenerator("base:tile/geothermal_generator"_id, position_) {}

	FluidAmount GeothermalGenerator::getMaxLevel(FluidID id) const {
		if (auto fluid = getGame().registry<FluidRegistry>().maybe(id))
			return fluid->identifier == "base:fluid/lava"_id? 16 * FluidTile::FULL : 0;
		return 0;
	}

	EnergyAmount GeothermalGenerator::getEnergyCapacity() {
		return 64'000;
	}

	void GeothermalGenerator::toJSON(nlohmann::json &json) const {
		TileEntity::toJSON(json);
		FluidHoldingTileEntity::toJSON(json);
		EnergeticTileEntity::toJSON(json);
	}

	bool GeothermalGenerator::onInteractNextTo(const PlayerPtr &player, Modifiers modifiers) {
		auto &realm = *getRealm();

		if (modifiers.onlyAlt()) {
			realm.queueDestruction(shared_from_this());
			player->give(ItemStack(realm.getGame(), "base:item/geothermal_generator"_id));
			return true;
		}

		if (modifiers.onlyCtrl())
			FluidHoldingTileEntity::addObserver(player);
		else
			EnergeticTileEntity::addObserver(player);

		{
			assert(fluidContainer);
			auto lock = fluidContainer->levels.sharedLock();
			if (fluidContainer->levels.empty())
				WARN("No fluids.");
			else
				for (const auto &[id, amount]: fluidContainer->levels)
					INFO(realm.getGame().getFluid(id)->identifier << " = " << amount);
		}

		std::shared_lock lock{energyMutex};
		INFO("Energy: " << energyAmount);
		return false;
	}

	void GeothermalGenerator::absorbJSON(Game &game, const nlohmann::json &json) {
		TileEntity::absorbJSON(game, json);
		FluidHoldingTileEntity::absorbJSON(game, json);
		EnergeticTileEntity::absorbJSON(game, json);
	}

	void GeothermalGenerator::encode(Game &game, Buffer &buffer) {
		TileEntity::encode(game, buffer);
		FluidHoldingTileEntity::encode(game, buffer);
		EnergeticTileEntity::encode(game, buffer);
	}

	void GeothermalGenerator::decode(Game &game, Buffer &buffer) {
		TileEntity::decode(game, buffer);
		FluidHoldingTileEntity::decode(game, buffer);
		EnergeticTileEntity::decode(game, buffer);
	}

	void GeothermalGenerator::broadcast() {
		assert(getSide() == Side::Server);

		const TileEntityPacket packet(shared_from_this());

		auto energetic_lock = EnergeticTileEntity::observers.uniqueLock();

		std::erase_if(EnergeticTileEntity::observers, [&](const std::weak_ptr<Player> &weak_player) {
			if (auto player = weak_player.lock()) {
				player->send(packet);
				return false;
			}

			return true;
		});

		auto fluid_holding_lock = FluidHoldingTileEntity::observers.uniqueLock();

		std::erase_if(FluidHoldingTileEntity::observers, [&](const std::weak_ptr<Player> &weak_player) {
			if (auto player = weak_player.lock()) {
				if (!EnergeticTileEntity::observers.contains(player))
					player->send(packet);
				return false;
			}

			return true;
		});
	}

	Game & GeothermalGenerator::getGame() const {
		return TileEntity::getGame();
	}
}
