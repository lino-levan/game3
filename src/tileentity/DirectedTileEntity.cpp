#include "tileentity/DirectedTileEntity.h"

namespace Game3 {
	DirectedTileEntity::DirectedTileEntity(Direction direction):
		tileDirection(direction) {}

	void DirectedTileEntity::setDirection(Direction new_direction) {
		tileDirection = new_direction;

		switch (tileDirection) {
			case Direction::Up:    tileID = Identifier(getDirectedTileBase() + 'n'); break;
			case Direction::Right: tileID = Identifier(getDirectedTileBase() + 'e'); break;
			case Direction::Down:  tileID = Identifier(getDirectedTileBase() + 's'); break;
			case Direction::Left:  tileID = Identifier(getDirectedTileBase() + 'w'); break;
			default:
				tileID = "base:tile/missing"_id;
		}

		cachedTile = -1;
	}

	void DirectedTileEntity::rotateClockwise() {
		setDirection(Game3::rotateClockwise(getDirection()));
		increaseUpdateCounter();
		queueBroadcast(true);
	}

	void DirectedTileEntity::rotateCounterClockwise() {
		setDirection(Game3::rotateCounterClockwise(getDirection()));
		increaseUpdateCounter();
		queueBroadcast(true);
	}

	void DirectedTileEntity::toJSON(nlohmann::json &json) const {
		json["direction"] = tileDirection;
	}

	void DirectedTileEntity::absorbJSON(Game &, const nlohmann::json &json) {
		setDirection(json.at("direction"));
	}

	void DirectedTileEntity::encode(Game &, Buffer &buffer) {
		buffer << getDirection();
	}

	void DirectedTileEntity::decode(Game &, Buffer &buffer) {
		setDirection(buffer.take<Direction>());
	}
}
