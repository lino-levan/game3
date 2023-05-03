#pragma once

#include "realm/Realm.h"
#include "worldgen/WorldGen.h"

namespace Game3 {
	class Overworld: public Realm {
		public:
			static Identifier ID() { return {"base", "realm/overworld"}; }

			WorldGenParams worldgenParams;

			using Realm::Realm;

			void generateChunk(const ChunkPosition &) override;

		protected:
			void absorbJSON(const nlohmann::json &) override;
			void toJSON(nlohmann::json &) const override;
	};
}
