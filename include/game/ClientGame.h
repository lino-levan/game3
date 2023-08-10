#pragma once

#include "game/Game.h"
#include "ui/Modifiers.h"

namespace Game3 {
	class HasEnergy;
	class HasFluids;
	class HasInventory;
	class LocalClient;
	class Packet;

	class ClientGame: public Game {
		public:
			Canvas &canvas;
			ClientPlayerPtr player;

			std::shared_ptr<LocalClient> client;
			RealmPtr activeRealm;

			ClientGame(Canvas &canvas_): Game(), canvas(canvas_) {}

			void addEntityFactories() override;
			void click(int button, int n, double pos_x, double pos_y, Modifiers);
			Gdk::Rectangle getVisibleRealmBounds() const;
			MainWindow & getWindow();
			/** Translates coordinates relative to the top left corner of the canvas to realm coordinates. */
			Position translateCanvasCoordinates(double x, double y, double *x_offset_out = nullptr, double *y_offset_out = nullptr) const;
			void activateContext();
			void setText(const Glib::ustring &text, const Glib::ustring &name = "", bool focus = true, bool ephemeral = false);
			const Glib::ustring & getText() const;
			void runCommand(const std::string &);
			void tick() final;
			void queuePacket(std::shared_ptr<Packet>);
			void chunkReceived(ChunkPosition);
			void interactOn(Modifiers);
			void interactNextTo(Modifiers);
			void putInLimbo(EntityPtr, RealmID, const Position &);
			void requestFromLimbo(RealmID);

			void moduleMessageBuffer(const Identifier &module_id, const std::shared_ptr<Agent> &source, const std::string &name, Buffer &&data);

			template <typename... Args>
			void moduleMessage(const Identifier &module_id, const std::shared_ptr<Agent> &source, const std::string &name, Args &&...args) {
				moduleMessageBuffer(module_id, source, name, Buffer{std::forward<Args>(args)...});
			}

			auto signal_player_inventory_update() const { return signal_player_inventory_update_; }
			auto signal_player_money_update()     const { return signal_player_money_update_;     }
			auto signal_other_inventory_update()  const { return signal_other_inventory_update_;  }
			auto signal_fluid_update()            const { return signal_fluid_update_;            }
			auto signal_energy_update()           const { return signal_energy_update_;           }

			Side getSide() const override { return Side::Client; }

		private:
			sigc::signal<void(const PlayerPtr &)> signal_player_inventory_update_;
			sigc::signal<void(const PlayerPtr &)> signal_player_money_update_;
			sigc::signal<void(const std::shared_ptr<Agent> &)> signal_other_inventory_update_;
			sigc::signal<void(const std::shared_ptr<HasFluids> &)> signal_fluid_update_;
			sigc::signal<void(const std::shared_ptr<HasEnergy> &)> signal_energy_update_;

			std::set<ChunkPosition> missingChunks;
			MTQueue<std::shared_ptr<Packet>> packetQueue;
			/** Temporarily stores shared pointers to entities that have moved to a realm we're unaware of to prevent destruction. */
			Lockable<std::unordered_map<RealmID, std::unordered_map<EntityPtr, Position>>> entityLimbo;
	};
}
