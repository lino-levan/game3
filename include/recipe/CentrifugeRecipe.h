#pragma once

#include "data/Identifier.h"
#include "game/Fluids.h"
#include "item/Item.h"
#include "recipe/Recipe.h"
#include "registry/Registries.h"

#include <nlohmann/json_fwd.hpp>

namespace Game3 {
	struct CentrifugeRecipe: Recipe<FluidStack, ItemStack> {
		Input input;
		std::map<nlohmann::json, double> weightMap;

		CentrifugeRecipe() = default;
		CentrifugeRecipe(Input, std::map<nlohmann::json, double>);

		Input getInput(Game &) override;
		Output getOutput(const Input &, Game &) override;
		/** Doesn't lock the container. */
		bool canCraft(const std::shared_ptr<Container> &) override;
		/** Doesn't lock either container. */
		bool craft(Game &, const std::shared_ptr<Container> &input_container, const std::shared_ptr<Container> &output_container, std::optional<Output> &leftovers) override;
		void toJSON(nlohmann::json &) const override;

		static CentrifugeRecipe fromJSON(const Game &, const nlohmann::json &);
	};

	struct CentrifugeRecipeRegistry: UnnamedJSONRegistry<CentrifugeRecipe> {
		static Identifier ID() { return {"base", "centrifuge_recipe"}; }
		CentrifugeRecipeRegistry(): UnnamedJSONRegistry(ID()) {}
	};
}
