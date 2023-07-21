#pragma once

#include "Position.h"
#include "Types.h"
#include "util/PairHash.h"
#include "util/WeakSet.h"

#include <memory>
#include <unordered_set>
#include <utility>

namespace Game3 {
	class Pipe;
	class Realm;

	class PipeNetwork: public std::enable_shared_from_this<PipeNetwork> {
		protected:
			using PairSet = std::unordered_set<std::pair<Position, Direction>, PairHash<Position, Direction>>;

			WeakSet<Pipe> members;
			size_t id = 0;
			std::weak_ptr<Realm> weakRealm;
			size_t lastTick = 0;

			PairSet extractions;
			PairSet insertions;

		public:
			PipeNetwork(size_t id_, const std::shared_ptr<Realm> &);

			virtual ~PipeNetwork() = default;

			static std::unique_ptr<PipeNetwork> create(PipeType, size_t id, const std::shared_ptr<Realm> &);

			void add(std::weak_ptr<Pipe>);
			void absorb(const std::shared_ptr<PipeNetwork> &);
			/** Cuts the network into two pieces by setting all pipes reachable from the given pipe to a new network. */
			void partition(const std::shared_ptr<Pipe> &);
			virtual void addExtraction(Position, Direction);
			virtual void addInsertion(Position, Direction);
			virtual bool removeExtraction(Position, Direction);
			virtual bool removeInsertion(Position, Direction);

			inline auto getID() const { return id; }

			virtual PipeType getType() const = 0;
			virtual void tick(Tick);
			bool canTick(Tick);
	};
}