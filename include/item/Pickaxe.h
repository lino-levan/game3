#pragma once

#include "item/Tool.h"

namespace Game3 {
	struct Pickaxe: Tool {
		using Tool::Tool;
		bool use(Slot, ItemStack &, const Place &, Modifiers, std::pair<float, float>) override;
		bool canUseOnWorld() const override { return true; }
	};
}
