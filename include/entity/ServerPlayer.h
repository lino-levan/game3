#pragma once

#include "entity/Player.h"
#include "threading/Lockable.h"
#include "container/WeakSet.h"

namespace Game3 {
	class ServerPlayer: public Player {
		public:
			Lockable<WeakSet<Entity>> knownEntities;
			std::weak_ptr<RemoteClient> weakClient;
			bool inventoryUpdated = false;

			~ServerPlayer() override;

			static std::shared_ptr<ServerPlayer> create(Game &);
			static std::shared_ptr<ServerPlayer> fromJSON(Game &, const nlohmann::json &);

			/** Returns true if the entity had to be sent. */
			bool ensureEntity(const std::shared_ptr<Entity> &);
			std::shared_ptr<RemoteClient> getClient() const;

			void handleMessage(const std::shared_ptr<Agent> &source, const std::string &name, std::any &data) final;

			void kill() override;

		private:
			ServerPlayer();

			friend class Entity;
	};
}
