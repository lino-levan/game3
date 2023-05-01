#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "Constants.h"
#include "Types.h"

namespace Game3 {
	struct ChunkPosition {
		int32_t x = 0;
		int32_t y = 0;

		bool operator==(const ChunkPosition &) const;
		explicit operator std::string() const;
	};

	struct ChunkRange {
		ChunkPosition topLeft;
		ChunkPosition bottomRight;

		inline Index tileWidth() const  { return (bottomRight.x - topLeft.x + 1) * CHUNK_SIZE; }
		inline Index tileHeight() const { return (bottomRight.y - topLeft.y + 1) * CHUNK_SIZE; }

		inline Index rowMin() const { return topLeft.y * CHUNK_SIZE; }
		/** Compare with <=, not <. */
		inline Index rowMax() const { return (bottomRight.y + 1) * CHUNK_SIZE - 1; }
		inline Index columnMin() const { return topLeft.x * CHUNK_SIZE; }
		/** Compare with <=, not <. */
		inline Index columnMax() const { return (bottomRight.x + 1) * CHUNK_SIZE - 1; }
	};
}

namespace std {
	template <>
	struct hash<Game3::ChunkPosition> {
		size_t operator()(const Game3::ChunkPosition &chunk_position) const {
			return (static_cast<size_t>(chunk_position.x) << 32) | static_cast<size_t>(chunk_position.y);
		}
	};
}