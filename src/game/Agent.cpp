#include "ThreadContext.h"
#include "game/Agent.h"
#include "realm/Realm.h"

namespace Game3 {
	std::vector<ChunkPosition> Agent::getVisibleChunks() const {
		ChunkPosition original_position = getChunkPosition(getPosition());
		std::vector<ChunkPosition> out;
		out.reserve(REALM_DIAMETER * REALM_DIAMETER);
		constexpr int32_t half = REALM_DIAMETER / 2;
		for (int32_t y = -half; y <= half; ++y)
			for (int32_t x = -half; x <= half; ++x)
				out.emplace_back(ChunkPosition{x + original_position.x, y + original_position.y});
		return out;
	}

	GlobalID Agent::generateGID() {
		return static_cast<GlobalID>(threadContext.rng());
	}
}
