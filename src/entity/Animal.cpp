#include "threading/ThreadContext.h"
#include "entity/Animal.h"
#include "game/Game.h"
#include "graphics/TextRenderer.h"
#include "net/Buffer.h"
#include "realm/Keep.h"
#include "tileentity/Building.h"
#include "tileentity/Chest.h"
#include "tileentity/Teleporter.h"

namespace Game3 {
	ThreadPool Animal::threadPool{2};

	namespace {
		constexpr HitPoints MAX_HEALTH = 40;
		constexpr float RETRY_TIME = 30;
	}

	Animal::Animal():
		Entity("base:invalid/Animal") {}

	bool Animal::onInteractNextTo(const std::shared_ptr<Player> &player, Modifiers, ItemStack *used_item, Hand) {
		if (!used_item || used_item->item->identifier != "base:item/wrench")
			return false;

		INFO(typeid(*this).name() << ' ' << getGID() << ':');
		INFO("  Path length is " << path.size());
		auto realm = getRealm();
		{
			auto lock = visibleEntities.sharedLock();
			INFO("  Player is visible? " << std::boolalpha << visiblePlayers.contains(player));
		}
		{
			auto lock = player->visibleEntities.sharedLock();
			INFO("  Visible to player? " << std::boolalpha << player->visibleEntities.contains(getSelf()));
		}
		if (auto ptr = realm->getEntities(getChunk()); ptr && ptr->contains(getSelf()))
			SUCCESS("  In chunk.");
		else
			ERROR("  Not in chunk.");
		INFO("  Time until wander: " << timeUntilWander.load());
		INFO("  Attempting wander: " << std::boolalpha << attemptingWander.load());
		return true;
	}

	void Animal::toJSON(nlohmann::json &json) const {
		Entity::toJSON(json);

		if (0.f < timeUntilWander)
			json["timeUntilWander"] = timeUntilWander.load();
	}

	void Animal::absorbJSON(Game &game, const nlohmann::json &json) {
		Entity::absorbJSON(game, json);

		if (auto iter = json.find("timeUntilWander"); iter != json.end())
			timeUntilWander = *iter;
	}

	void Animal::init(Game &game) {
		Entity::init(game);
		threadPool.start();
	}

	void Animal::tick(Game &game, float delta) {
		Entity::tick(game, delta);

		if (getSide() == Side::Server) {
			timeUntilWander = timeUntilWander - delta;
			if (!attemptingWander && timeUntilWander <= 0.f)
				wander();
		}
	}

	HitPoints Animal::getMaxHealth() const {
		return MAX_HEALTH;
	}

	bool Animal::wander() {
		if (!attemptingWander.exchange(true)) {
			increaseUpdateCounter();
			const auto [row, column] = position.copyBase();
			return threadPool.add([this, row = row, column = column](ThreadPool &, size_t) {
				pathfind({
					threadContext.random(int64_t(row    - wanderRadius), int64_t(row    + wanderRadius)),
					threadContext.random(int64_t(column - wanderRadius), int64_t(column + wanderRadius))
				}, 256);

				timeUntilWander = getWanderDistribution()(threadContext.rng);
				attemptingWander = false;
			});
		}

		return false;
	}

	void Animal::encode(Buffer &buffer) {
		Entity::encode(buffer);
		LivingEntity::encode(buffer);
		buffer << timeUntilWander.load();
		buffer << wanderRadius;
	}

	void Animal::decode(Buffer &buffer) {
		Entity::decode(buffer);
		LivingEntity::decode(buffer);
		timeUntilWander = buffer.take<float>();
		buffer >> wanderRadius;
	}
}
