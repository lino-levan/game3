#include "graphics/Tileset.h"
#include "game/ClientGame.h"
#include "game/Fluids.h"
#include "game/Game.h"
#include "game/InteractionSet.h"
#include "game/Inventory.h"
#include "game/ServerGame.h"
#include "graphics/ItemTexture.h"
#include "ui/module/AutocrafterModule.h"
#include "ui/module/ChemicalReactorModule.h"
#include "ui/module/CombinerModule.h"
#include "ui/module/InventoryModule.h"
#include "ui/module/EnergyLevelModule.h"
#include "ui/module/FluidLevelsModule.h"
#include "ui/module/ItemFilterModule.h"
#include "ui/module/ModuleFactory.h"
#include "algorithm/AStar.h"
#include "util/FS.h"
#include "util/Timer.h"
#include "util/Util.h"

#include <nlohmann/json.hpp>

namespace Game3 {
	Game::~Game() {
		INFO("\e[31m~Game\e[39m(" << this << ')');
		dying = true;
	}

	bool Game::tick() {
		auto now = getTime();
		auto difference = now - lastTime;
		lastTime = now;
		delta = std::chrono::duration_cast<std::chrono::nanoseconds>(difference).count() / 1e9;
		time = time + delta;
		++currentTick;
		return true;
	}

	void Game::addModuleFactories() {
		add(ModuleFactory::create<InventoryModule>());
		add(ModuleFactory::create<FluidLevelsModule>());
		add(ModuleFactory::create<ChemicalReactorModule>());
		add(ModuleFactory::create<EnergyLevelModule>());
		add(ModuleFactory::create<ItemFilterModule>());
		add(ModuleFactory::create<CombinerModule>());
		add(ModuleFactory::create<AutocrafterModule>());
	}

	void Game::initialSetup(const std::filesystem::path &dir) {
		initRegistries();
		addItems();
		traverseData(dataRoot / dir);
		addRealms();
		addEntityFactories();
		addTileEntityFactories();
		addPacketFactories();
		addLocalCommandFactories();
		addTiles();
		addModuleFactories();
	}

	void Game::initEntities() {
		for (const auto &[realm_id, realm]: realms)
			realm->initEntities();
	}

	void Game::initInteractionSets() {
		interactionSets.clear();
		auto standard = std::make_shared<StandardInteractions>();
		for (const auto &type: registry<RealmTypeRegistry>().items)
			interactionSets.emplace(type, standard);
	}

	void Game::add(std::shared_ptr<Item> item) {
		registry<ItemRegistry>().add(item->identifier, item);
		for (const auto &attribute: item->attributes)
			itemsByAttribute[attribute].insert(item);
	}

	void Game::add(ModuleFactory &&factory) {
		auto shared = std::make_shared<ModuleFactory>(std::move(factory));
		registry<ModuleFactoryRegistry>().add(shared->identifier, shared);
	}

	void Game::addRecipe(const nlohmann::json &json) {
		const Identifier identifier = json.at(0);
		if (identifier.getPathStart() != "ignore")
			registries.at(identifier)->toUnnamed()->add(*this, json.at(1));
	}

	RealmID Game::newRealmID() const {
		// TODO: a less stupid way of doing this.
		RealmID max = 1;
		for (const auto &[id, realm]: realms)
			max = std::max(max, id);
		return max + 1;
	}

	double Game::getTotalSeconds() const {
		return time;
	}

	double Game::getHour() const {
		const auto base = time / 10. + hourOffset;
		return int64_t(base) % 24 + fractional(base);
	}

	double Game::getMinute() const {
		return 60. * fractional(getHour());
	}

	double Game::getDivisor() const {
		return 3. - 2. * std::sin(getHour() * M_PI / 24.);
	}

	std::optional<TileID> Game::getFluidTileID(FluidID fluid_id) {
		if (auto iter = fluidCache.find(fluid_id); iter != fluidCache.end())
			return iter->second;

		if (auto fluid = registry<FluidRegistry>().maybe(static_cast<size_t>(fluid_id))) {
			if (auto tileset = registry<TilesetRegistry>().maybe(fluid->tilesetName)) {
				if (auto fluid_tileid = tileset->maybe(fluid->tilename)) {
					fluidCache.emplace(fluid_id, *fluid_tileid);
					return *fluid_tileid;
				}
			}
		}

		return std::nullopt;
	}

	std::shared_ptr<Fluid> Game::getFluid(FluidID fluid_id) const {
		return registry<FluidRegistry>().maybe(static_cast<size_t>(fluid_id));
	}

	GamePtr Game::create(Side side, const GameArgument &argument) {
		GamePtr out;
		if (side == Side::Client) {
			out = GamePtr(new ClientGame(*std::get<Canvas *>(argument)));
		} else {
			const auto [server_ptr, pool_size] = std::get<std::pair<std::shared_ptr<Server>, size_t>>(argument);
			out = GamePtr(new ServerGame(server_ptr, pool_size));
		}
		out->initialSetup();
		return out;
	}

	GamePtr Game::fromJSON(Side side, const nlohmann::json &json, const GameArgument &argument) {
		auto out = create(side, argument);
		out->initialSetup();
		{
			auto lock = out->realms.uniqueLock();
			for (const auto &[string, realm_json]: json.at("realms").get<std::unordered_map<std::string, nlohmann::json>>())
				out->realms.emplace(parseUlong(string), Realm::fromJSON(*out, realm_json));
		}
		out->hourOffset = json.contains("hourOffset")? json.at("hourOffset").get<float>() : 0.f;
		out->debugMode = json.contains("debugMode")? json.at("debugMode").get<bool>() : false;
		out->cavesGenerated = json.contains("cavesGenerated")? json.at("cavesGenerated").get<decltype(Game::cavesGenerated)>() : 0;
		return out;
	}

	ClientGame & Game::toClient() {
		return dynamic_cast<ClientGame &>(*this);
	}

	const ClientGame & Game::toClient() const {
		return dynamic_cast<const ClientGame &>(*this);
	}

	std::shared_ptr<ClientGame> Game::toClientPointer() {
		assert(getSide() == Side::Client);
		return std::static_pointer_cast<ClientGame>(shared_from_this());
	}

	ServerGame & Game::toServer() {
		return dynamic_cast<ServerGame &>(*this);
	}

	const ServerGame & Game::toServer() const {
		return dynamic_cast<const ServerGame &>(*this);
	}

	std::shared_ptr<ServerGame> Game::toServerPointer() {
		assert(getSide() == Side::Server);
		return std::static_pointer_cast<ServerGame>(shared_from_this());
	}

	void to_json(nlohmann::json &json, const Game &game) {
		json["debugMode"] = game.debugMode;
		json["realms"] = std::unordered_map<std::string, nlohmann::json>();
		game.iterateRealms([&](const RealmPtr &realm) {
			realm->toJSON(json["realms"][std::to_string(realm->id)], true);
		});
		json["hourOffset"] = game.getHour();
		if (0 < game.cavesGenerated)
			json["cavesGenerated"] = game.cavesGenerated;
	}

	template <>
	std::shared_ptr<Agent> Game::getAgent<Agent>(GlobalID gid) {
		auto shared_lock = allAgents.sharedLock();
		if (auto iter = allAgents.find(gid); iter != allAgents.end()) {
			if (auto agent = iter->second.lock())
				return agent;
			// This should *probably* not result in a data race in practice...
			shared_lock.unlock();
			auto unique_lock = allAgents.uniqueLock();
			allAgents.erase(gid);
		}

		return nullptr;
	}
}
