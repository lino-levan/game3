#pragma once

#include "threading/Lockable.h"
#include "tileentity/EnergeticTileEntity.h"
#include "tileentity/InventoriedTileEntity.h"

namespace Game3 {
	struct CraftingRecipe;

	class Autocrafter: public InventoriedTileEntity, public EnergeticTileEntity {
		public:
			static Identifier ID() { return {"base", "te/autocrafter"}; }

			std::string getName() const override { return "Autocrafter"; }

			const std::shared_ptr<Inventory> & getInventory(InventoryID) const override;
			void setInventory(std::shared_ptr<Inventory>, InventoryID) override;

			bool mayInsertItem(const ItemStack &, Direction, Slot) override;
			bool mayExtractItem(Direction, Slot) override;
			EnergyAmount getEnergyCapacity() override;

			void init(Game &) override;
			void tick(Game &, float) override;
			bool onInteractNextTo(const std::shared_ptr<Player> &, Modifiers) override;

			void toJSON(nlohmann::json &) const override;
			void absorbJSON(Game &, const nlohmann::json &) override;

			void encode(Game &, Buffer &) override;
			void decode(Game &, Buffer &) override;
			void broadcast(bool force) override;

			void handleMessage(const std::shared_ptr<Agent> &source, const std::string &name, std::any &data) override;

			const auto & getTarget() const { return target; }
			const auto & getStationInventory() const { return stationInventory; }

		private:
			float accumulatedTime = 0.f;
			Lockable<std::vector<std::shared_ptr<CraftingRecipe>>> cachedRecipes;
			Lockable<Identifier> target;
			Lockable<std::shared_ptr<Inventory>> stationInventory;
			Lockable<Identifier> station;

			Autocrafter();
			Autocrafter(Identifier tile_id, Position);
			Autocrafter(Position);

			void autocraft();
			bool setTarget(Identifier);
			void cacheRecipes();
			bool stationSet();
			bool validateRecipe(const CraftingRecipe &) const;

			friend class TileEntity;
	};
}
