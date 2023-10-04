#pragma once

#include "realm/Realm.h"

#include <atomic>

namespace Game3 {
	class Inventory;

	class Cave: public Realm {
		public:
			static Identifier ID() { return {"base", "realm/cave"}; }
			RealmID parentRealm;
			std::atomic_size_t entranceCount = 1;

			Cave(const Cave &) = delete;
			Cave(Cave &&) = delete;

			Cave & operator=(const Cave &) = delete;
			Cave & operator=(Cave &&) = delete;

			void onRemove() override;
			bool interactGround(const std::shared_ptr<Player> &, const Position &, Modifiers) override;
			void reveal(const Position &);
			void generateChunk(const ChunkPosition &) override;

			friend class Realm;

		protected:
			using Realm::Realm;

			Cave() = delete;
			Cave(Game &, RealmID, RealmID parent_realm, int seed_);

			void absorbJSON(const nlohmann::json &, bool full_data) override;
			void toJSON(nlohmann::json &, bool full_data) const override;
	};
}
