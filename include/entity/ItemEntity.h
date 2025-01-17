#pragma once

#include "graphics/Texture.h"
#include "entity/Entity.h"
#include "item/Item.h"
#include "threading/Atomic.h"

#include <nlohmann/json_fwd.hpp>

namespace Game3 {
	class Game;

	class ItemEntity: public Entity {
		public:
			static Identifier ID() { return {"base", "entity/item"}; }
			const ItemStack & getStack() const { return stack; }
			void setStack(ItemStack);

			static std::shared_ptr<ItemEntity> create(Game &);
			static std::shared_ptr<ItemEntity> create(Game &, ItemStack);
			static std::shared_ptr<ItemEntity> fromJSON(Game &, const nlohmann::json &);

			void toJSON(nlohmann::json &) const override;
			void init(Game &) override;
			void tick(Game &, float) override;
			void render(const RendererContext &) override;
			bool onInteractOn    (const std::shared_ptr<Player> &player, Modifiers, ItemStack *, Hand) override { return interact(player); }
			bool onInteractNextTo(const std::shared_ptr<Player> &player, Modifiers, ItemStack *, Hand) override { return interact(player); }
			std::string getName() const override;
			void encode(Buffer &) override;
			void decode(Buffer &) override;
			int getZIndex() const override { return -1; }

		private:
			ItemEntity(): Entity(ID()) {}
			ItemEntity(const Game &);
			ItemEntity(ItemStack);
			float offsetX = 0.f;
			float offsetY = 0.f;
			float sizeX = 16.f;
			float sizeY = 16.f;
			bool needsTexture = true;
			Atomic<float> secondsLeft = 5 * 60;

			ItemStack stack;

			void setTexture(const Game &);
			bool interact(const std::shared_ptr<Player> &);

		friend class Entity;
	};

	void to_json(nlohmann::json &, const ItemEntity &);
}
