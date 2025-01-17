#pragma once

#include "net/Buffer.h"
#include "packet/Packet.h"

namespace Game3 {
	class Entity;

	class EntityPacket: public Packet {
		public:
			static PacketID ID() { return 14; }

			std::shared_ptr<Entity> entity;
			Identifier identifier;
			GlobalID globalID = -1;
			RealmID realmID = -1;

			EntityPacket() = default;
			EntityPacket(std::shared_ptr<Entity>);

			PacketID getID() const override { return ID(); }

			void encode(Game &, Buffer &) const override;
			void decode(Game &, Buffer &) override;

			void handle(ClientGame &) override;

		private:
			Buffer storedBuffer;
			bool wasFound = false;
	};
}
