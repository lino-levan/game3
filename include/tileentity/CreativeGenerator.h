#pragma once

#include "graphics/Texture.h"
#include "tileentity/EnergeticTileEntity.h"
#include "tileentity/FluidHoldingTileEntity.h"
#include "tileentity/InventoriedTileEntity.h"

namespace Game3 {
	class CreativeGenerator: public EnergeticTileEntity {
		public:
			static Identifier ID() { return {"base", "te/creative_generator"}; }

			EnergyAmount getEnergyCapacity() override;

			std::string getName() const override { return "Creative Generator"; }

			void init(Game &) override;
			void tick(Game &, float) override;
			void toJSON(nlohmann::json &) const override;
			bool onInteractNextTo(const std::shared_ptr<Player> &, Modifiers, ItemStack *, Hand) override;
			void absorbJSON(Game &, const nlohmann::json &) override;

			void encode(Game &, Buffer &) override;
			void decode(Game &, Buffer &) override;
			void broadcast(bool force) override;

		private:
			float accumulatedTime = 0.f;
			Lockable<std::optional<std::unordered_set<FluidID>>> supportedFluids;

			CreativeGenerator();
			CreativeGenerator(Identifier tile_id, Position);
			CreativeGenerator(Position);

			void slurpFlasks();

			friend class TileEntity;
	};
}
