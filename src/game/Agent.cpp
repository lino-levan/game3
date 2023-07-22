#include <mutex>
#include <random>

#include "Log.h"
#include "game/Agent.h"
#include "realm/Realm.h"

namespace Game3 {
	static std::default_random_engine agent_rng(42);
	// static std::random_device agent_rng;
	static std::mutex agent_rng_mutex;

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

	bool Agent::validateGID(GlobalID gid) {
		return gid != 0 && gid != static_cast<GlobalID>(-1);
	}

	GlobalID Agent::generateGID() {
		std::unique_lock lock(agent_rng_mutex);
		return static_cast<GlobalID>(agent_rng());
	}

	bool Agent::hasBeenSentTo(const std::shared_ptr<Player> &player) {
		auto lock = sentTo.sharedLock();
		return sentTo.contains(player);
	}

	void Agent::onSend(const std::shared_ptr<Player> &player) {
		auto lock = sentTo.uniqueLock();
		sentTo.emplace(player);
	}
}
