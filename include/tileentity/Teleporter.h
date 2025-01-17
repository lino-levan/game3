#pragma once

#include "tileentity/TileEntity.h"

namespace Game3 {
	class Teleporter: public TileEntity {
		public:
			static Identifier ID() { return {"base", "te/teleporter"}; }
			RealmID targetRealm;
			Position targetPosition;

			std::string getName() const override { return "Teleporter"; }

			void toJSON(nlohmann::json &) const override;
			void absorbJSON(Game &, const nlohmann::json &) override;
			void onOverlap(const std::shared_ptr<Entity> &) override;

			void encode(Game &, Buffer &) override;
			void decode(Game &, Buffer &) override;

		protected:
			Teleporter() = default;
			Teleporter(Identifier tilename, Position position_, RealmID target_realm, Position target_position);

			friend class TileEntity;
	};
}
