#include <iostream>

#include "Texture.h"
#include "Tileset.h"
#include "entity/Player.h"
#include "game/ClientGame.h"
#include "game/ServerInventory.h"
#include "realm/Realm.h"
#include "tileentity/Chest.h"
#include "ui/SpriteRenderer.h"
#include "ui/tab/InventoryTab.h"

namespace Game3 {
	Chest::Chest(Identifier tile_id, const Position &position_, std::string name_):
		TileEntity(std::move(tile_id), ID(), position_, true), name(std::move(name_)) {}

	Chest::Chest(const Position &position_):
		Chest("base:tile/chest"_id, position_, "Chest") {}

	std::string Chest::getName() {
		return name;
	}

	void Chest::toJSON(nlohmann::json &json) const {
		TileEntity::toJSON(json);
		if (inventory)
			json["inventory"] = dynamic_cast<ServerInventory &>(*inventory);
		json["name"] = name;
	}

	bool Chest::onInteractNextTo(const std::shared_ptr<Player> &player, Modifiers) {
		addObserver(player, false);
		return true;
	}

	void Chest::absorbJSON(Game &game, const nlohmann::json &json) {
		TileEntity::absorbJSON(game, json);
		if (json.contains("inventory"))
			inventory = std::make_shared<ServerInventory>(ServerInventory::fromJSON(game, json.at("inventory"), shared_from_this()));
		name = json.at("name");
	}

	void Chest::encode(Game &game, Buffer &buffer) {
		TileEntity::encode(game, buffer);
		InventoriedTileEntity::encode(game, buffer);
		buffer << name;
	}

	void Chest::decode(Game &game, Buffer &buffer) {
		TileEntity::decode(game, buffer);
		InventoriedTileEntity::decode(game, buffer);
		buffer >> name;
	}
}
