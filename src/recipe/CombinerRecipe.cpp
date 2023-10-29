#include "game/Game.h"
#include "game/Inventory.h"
#include "recipe/CombinerRecipe.h"
#include "threading/ThreadContext.h"
#include "util/Util.h"

#include <nlohmann/json.hpp>

namespace Game3 {
	CombinerRecipe::CombinerRecipe(Identifier identifier_):
		Recipe(std::move(identifier_)) {}

	CombinerRecipe::CombinerRecipe(Identifier identifier_, Game &game, const nlohmann::json &json):
		Recipe(std::move(identifier_)), input(CombinerInput::fromJSON(json, &outputCount).getStacks(game)) {}

	CombinerRecipe::Input CombinerRecipe::getInput(Game &) {
		return input;
	}

	CombinerRecipe::Output CombinerRecipe::getOutput(const Input &, Game &game) {
		return ItemStack(game, identifier, outputCount);
	}

	bool CombinerRecipe::canCraft(const std::shared_ptr<Container> &container) {
		if (auto inventory = std::dynamic_pointer_cast<Inventory>(container)) {
			// Assumption: the same type of item won't be listed multiple times in one CombinerInput::inputs.
			for (const ItemStack &stack: input)
				if (!inventory->contains(stack))
					return false;
		}

		return true;
	}

	bool CombinerRecipe::craft(Game &game, const std::shared_ptr<Container> &input_container, const std::shared_ptr<Container> &output_container, std::optional<Output> &leftover) {
		auto input_inventory  = std::dynamic_pointer_cast<Inventory>(input_container);
		auto output_inventory = std::dynamic_pointer_cast<Inventory>(output_container);

		if (!input_inventory || !output_inventory || !canCraft(input_inventory))
			return false;

		Output output = getOutput(input, game);

		ItemStack output_stack(game, identifier, outputCount);

		if (!output_inventory->canInsert(output_stack))
			return false;

		for (const ItemStack &input_stack: input)
			assert(input_stack.count == input_inventory->remove(input_stack));

		if (std::optional<ItemStack> insertion_leftover = output_inventory->add(output_stack))
			leftover = std::move(insertion_leftover);
		else
			leftover.reset();

		return true;
	}
}