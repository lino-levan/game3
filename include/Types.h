#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

#include "data/Identifier.h"
#include "registry/Registerable.h"

namespace Game3 {
	using Index        =  int64_t;
	using TileID       = uint16_t;
	using PlayerID     =  int32_t;
	using RealmID      =  int32_t;
	using Slot         =  int32_t;
	using ItemCount    = uint64_t;
	using MoneyCount   = uint64_t;
	using Phase        =  uint8_t;
	using Durability   =  int32_t;
	using BiomeType    = uint32_t;
	/** Number of quarter-hearts. */
	using HitPoints    = uint32_t;
	/** 1-based. */
	using Layer        = uint8_t;
	using PacketID     = uint16_t;
	using Version      = uint32_t;
	using GlobalID     = uint64_t;
	using Token        = uint64_t;

	using ItemID     = Identifier;
	using EntityType = Identifier;
	using RealmType  = Identifier;

	class Player;
	using PlayerPtr = std::shared_ptr<Player>;

	struct Place;

	enum class Side {Invalid, Server, Client};

	template <typename T>
	class NamedNumeric: public NamedRegisterable {
		public:
			NamedNumeric(Identifier identifier_, T value_):
				NamedRegisterable(std::move(identifier_)),
				value(value_) {}

			operator T() const { return value; }

		private:
			T value;
	};

	struct NamedDurability: NamedNumeric<Durability> {
		using NamedNumeric::NamedNumeric;
	};

	Index operator""_idx(unsigned long long);

	enum class PathResult: uint8_t {Invalid, Trivial, Unpathable, Success};

	struct Color {
		float red   = 0.f;
		float green = 0.f;
		float blue  = 0.f;
		float alpha = 1.f;

		Color() = default;
		Color(float red_, float green_, float blue_, float alpha_ = 1.f):
			red(red_), green(green_), blue(blue_), alpha(alpha_) {}
	};
}
