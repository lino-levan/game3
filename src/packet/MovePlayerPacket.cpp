#include "entity/ServerPlayer.h"
#include "game/ServerGame.h"
#include "net/RemoteClient.h"
#include "packet/ErrorPacket.h"
#include "packet/MovePlayerPacket.h"
#include "packet/PacketError.h"

namespace Game3 {
	MovePlayerPacket::MovePlayerPacket(const Position &position_, Direction movement_direction, std::optional<Direction> facing_direction, std::optional<Vector3> offset_):
		position(position_),
		movementDirection(movement_direction),
		facingDirection(facing_direction),
		offset(offset_) {}

	void MovePlayerPacket::handle(ServerGame &, RemoteClient &client) {
		if (auto player = client.getPlayer()) {
			player->path.clear();
			player->move(movementDirection, {.excludePlayerSelf = true, .clearOffset = false, .facingDirection = facingDirection, .forcedPosition = position, .forcedOffset = offset});
			return;
		}

		client.send(ErrorPacket("Can't move: no player"));
	}
}
