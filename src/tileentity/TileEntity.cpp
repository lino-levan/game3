#include "entity/ClientPlayer.h"
#include "graphics/Tileset.h"
#include "game/ClientGame.h"
#include "game/ServerGame.h"
#include "net/Buffer.h"
#include "net/RemoteClient.h"
#include "packet/TileEntityPacket.h"
#include "realm/Realm.h"
#include "tileentity/TileEntity.h"
#include "tileentity/TileEntityFactory.h"
#include "ui/Canvas.h"
#include "util/Cast.h"

namespace Game3 {
	void TileEntity::destroy() {
		RealmPtr realm = getRealm();
		assert(realm);
		TileEntityPtr self = getSelf();
		realm->removeSafe(self);

		if (getSide() == Side::Server) {
			ServerGame &game = realm->getGame().toServer();
			game.database.deleteTileEntity(self);
			game.tileEntityDestroyed(*this);
		}
	}

	std::shared_ptr<TileEntity> TileEntity::fromJSON(Game &game, const nlohmann::json &json) {
		auto factory = game.registry<TileEntityFactoryRegistry>().at(json.at("id").get<Identifier>());
		assert(factory);
		auto out = (*factory)();
		out->absorbJSON(game, json);
		return out;
	}

	void TileEntity::init(Game &game) {
		assert(!initialized);
		initialized = true;

		auto lock = game.allAgents.uniqueLock();
		assert(!game.allAgents.contains(globalID));
		game.allAgents[globalID] = shared_from_this();
	}

	void TileEntity::tick(Game &, float) {
		if (needsBroadcast.exchange(false))
			broadcast(forceBroadcast.exchange(false));
	}

	void TileEntity::render(SpriteRenderer &sprite_renderer) {
		if (!isVisible())
			return;

		RealmPtr realm = getRealm();
		Tileset &tileset = realm->getTileset();

		if (cachedTile == TileID(-1) || tileLookupFailed) {
			if (tileID.empty()) {
				tileLookupFailed = true;
				cachedTile = 0;
				cachedUpperTile = 0;
			} else {
				tileLookupFailed = false;
				cachedTile = tileset[tileID];
				cachedUpperTile = tileset.getUpper(cachedTile);
				if (cachedUpperTile == 0)
					cachedUpperTile = -1;
			}
		}

		if (cachedTile == 0)
			return;

		const auto tilesize = tileset.getTileSize();
		const auto texture = tileset.getTexture(realm->getGame());
		const auto x = (cachedTile % (texture->width / tilesize)) * tilesize;
		const auto y = (cachedTile / (texture->width / tilesize)) * tilesize;

		sprite_renderer(texture, {
			.x = float(position.column),
			.y = float(position.row),
			.offsetX = x / 2.f,
			.offsetY = y / 2.f,
			.sizeX = float(tilesize),
			.sizeY = float(tilesize),
		});
	}

	void TileEntity::renderUpper(SpriteRenderer &sprite_renderer) {
		if (!isVisible({-1, 0}))
			return;

		RealmPtr realm = getRealm();
		Tileset &tileset = realm->getTileset();

		if (cachedUpperTile == TileID(-1) || cachedUpperTile == 0)
			return;

		const auto tilesize = tileset.getTileSize();
		const auto texture = tileset.getTexture(realm->getGame());
		const auto x = (cachedUpperTile % (texture->width / tilesize)) * tilesize;
		const auto y = (cachedUpperTile / (texture->width / tilesize)) * tilesize;

		sprite_renderer(texture, {
			.x = float(position.column),
			.y = float(position.row - 1),
			.offsetX = x / 2.f,
			.offsetY = y / 2.f,
			.sizeX = float(tilesize),
			.sizeY = float(tilesize),
		});
	}

	void TileEntity::renderLighting(const RendererContext &) {}

	void TileEntity::onSpawn() {
		Game &game = getRealm()->getGame();
		if (game.getSide() == Side::Server)
			game.toServer().tileEntitySpawned(getSelf());
	}

	void TileEntity::onRemove() {
		Game &game = getRealm()->getGame();
		if (game.getSide() == Side::Client)
			game.toClient().moduleMessage({}, shared_from_this(), "TileEntityRemoved");
	}

	void TileEntity::setRealm(const RealmPtr &realm) {
		realmID = realm->id;
		weakRealm = realm;
	}

	RealmPtr TileEntity::getRealm() const {
		RealmPtr out = weakRealm.lock();
		if (!out)
			throw std::runtime_error("Couldn't lock tile entity's realm");
		return out;
	}

	void TileEntity::updateNeighbors() const {
		getRealm()->updateNeighbors(position, Layer::Submerged);
		getRealm()->updateNeighbors(position, Layer::Objects);
	}

	bool TileEntity::isVisible() const {
		const Position pos = getPosition();
		RealmPtr realm = getRealm();
		if (getSide() == Side::Client) {
			ClientGame &client_game = realm->getGame().toClient();
			return client_game.canvas.inBounds(pos) && ChunkRange(client_game.player->getChunk()).contains(pos.getChunk());
		}
		return realm->isVisible(pos);
	}

	bool TileEntity::isVisible(const Position &offset) const {
		const Position pos = getPosition() + offset;
		RealmPtr realm = getRealm();
		if (getSide() == Side::Client) {
			ClientGame &client_game = realm->getGame().toClient();
			return client_game.canvas.inBounds(pos) && ChunkRange(client_game.player->getChunk()).contains(pos.getChunk());
		}
		return realm->isVisible(pos);
	}

	Side TileEntity::getSide() const {
		return getRealm()->getGame().getSide();
	}

	ChunkPosition TileEntity::getChunk() const {
		return getPosition().getChunk();
	}

	Game & TileEntity::getGame() const {
		if (RealmPtr realm = weakRealm.lock())
			return realm->getGame();
		throw std::runtime_error("Couldn't get Game from TileEntity: couldn't lock Realm");
	}

	std::shared_ptr<TileEntity> TileEntity::getSelf() {
		return std::static_pointer_cast<TileEntity>(shared_from_this());
	}

	void TileEntity::encode(Game &, Buffer &buffer) {
		buffer << tileEntityID;
		buffer << tileID;
		buffer << position;
		buffer << solid;
		buffer << getUpdateCounter();
		buffer << extraData.dump();
	}

	void TileEntity::decode(Game &, Buffer &buffer) {
		buffer >> tileEntityID;
		buffer >> tileID;
		buffer >> position;
		buffer >> solid;
		setUpdateCounter(buffer.take<UpdateCounter>());
		extraData = nlohmann::json::parse(buffer.take<std::string>());
		cachedTile = -1;
	}

	void TileEntity::sendTo(RemoteClient &client, UpdateCounter threshold) {
		assert(getSide() == Side::Server);
		if (threshold == 0 || getUpdateCounter() < threshold) {
			client.send(TileEntityPacket(getSelf()));
			onSend(client.getPlayer());
		}
	}

	void TileEntity::broadcast(bool) {
		assert(getSide() == Side::Server);

		RealmPtr realm = getRealm();
		TileEntityPacket packet(getSelf());

		ChunkRange(getChunk()).iterate([&](ChunkPosition chunk_position) {
			if (auto entities = realm->getEntities(chunk_position)) {
				auto lock = entities->sharedLock();
				for (const auto &entity: *entities)
					if (entity->isPlayer())
						safeDynamicCast<Player>(entity)->send(packet);
			}
		});
	}

	void TileEntity::absorbJSON(Game &, const nlohmann::json &json) {
		assert(getSide() == Side::Server);
		tileEntityID = json.at("id");
		globalID     = json.at("gid");
		tileID       = json.at("tileID");
		position     = json.at("position");
		solid        = json.at("solid");
		if (auto iter = json.find("extra"); iter != json.end())
			extraData = *iter;
		increaseUpdateCounter();
	}

	void TileEntity::toJSON(nlohmann::json &json) const {
		json["id"]       = getID();
		json["gid"]      = globalID;
		json["tileID"]   = tileID;
		json["position"] = position;
		json["solid"]    = solid;
		if (!extraData.empty())
			json["extra"] = extraData;
	}

	bool TileEntity::spawnIn(const Place &place) {
		return place.realm->add(getSelf()) != nullptr;
	}

	void to_json(nlohmann::json &json, const TileEntity &tile_entity) {
		tile_entity.toJSON(json);
	}
}
