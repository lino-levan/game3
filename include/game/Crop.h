#pragma once

#include "Types.h"
#include "item/Item.h"

#include <nlohmann/json_fwd.hpp>

namespace Game3 {
	class Game;

	class Crop: public NamedRegisterable {
		public:
			Identifier customType;
			std::vector<Identifier> stages;
			ItemStack product;
			double chance;
			// TODO: refactor into subclasses with CropFactory
			nlohmann::json customData;

			Crop(Identifier, Identifier custom_type, std::vector<Identifier> stages_, ItemStack product_, double chance_, nlohmann::json custom_data);
			Crop(Identifier, Game &, const nlohmann::json &);

		private:
			static Identifier getCustomType(const nlohmann::json &);
			static nlohmann::json getCustomData(const nlohmann::json &);
	};
}
