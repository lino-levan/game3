#pragma once

#include "registry/Registerable.h"

#include <nlohmann/json_fwd.hpp>

#include <memory>
#include <optional>

namespace Game3 {
	class Game;
	struct Container;

	template <typename I, typename O, typename R = Registerable>
	struct Recipe: R {
		using Input  = I;
		using Output = O;

		using R::R;

		virtual ~Recipe() = default;

		/** Returns a copy of the ingredient of the recipe. */
		virtual Input getInput(Game &) = 0;

		/** Returns a copy of the result of the recipe for a given input. */
		virtual Output getOutput(const Input &, Game &) = 0;

		virtual bool canCraft(const std::shared_ptr<Container> &) = 0;

		/** Attempts to produce the result of the recipe, removing any ingredients from the given container as necessary. */
		virtual bool craft(Game &, const std::shared_ptr<Container> &input_container, const std::shared_ptr<Container> &output_container, std::optional<Output> &leftovers) = 0;

		virtual void toJSON(nlohmann::json &) const = 0;

		virtual bool operator==(const Recipe &other) {
			return this == &other;
		}
	};
}
