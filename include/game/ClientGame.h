#pragma once

#include "game/Game.h"
#include "threading/LockableSharedPtr.h"
#include "ui/Modifiers.h"
#include "ui/Sound.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <thread>

namespace Game3 {
	class HasEnergy;
	class HasFluids;
	class HasInventory;
	class LocalClient;
	class Packet;

	class ClientGame: public Game {
		public:
			Canvas &canvas;
			LockableSharedPtr<ClientPlayer> player;
			LockableSharedPtr<LocalClient> client;
			LockableSharedPtr<Realm> activeRealm;
			bool stoppedByError = false;
			std::function<void()> errorCallback;
			SoundProvider sounds;

			ClientGame(Canvas &canvas_): Game(), canvas(canvas_) {}

			void addEntityFactories() override;
			void click(int button, int n, double pos_x, double pos_y, Modifiers);
			void dragStart(const Position &, Modifiers);
			void dragUpdate(const Position &, Modifiers);
			void dragEnd(const Position &, Modifiers);
			void drag(const Position &, Modifiers);
			Gdk::Rectangle getVisibleRealmBounds() const;
			MainWindow & getWindow();
			/** Translates coordinates relative to the top left corner of the canvas to realm coordinates. */
			Position translateCanvasCoordinates(double x, double y, double *x_offset_out = nullptr, double *y_offset_out = nullptr) const;
			void activateContext();
			void setText(const Glib::ustring &text, const Glib::ustring &name = "", bool focus = true, bool ephemeral = false);
			const Glib::ustring & getText() const;
			void runCommand(const std::string &);
			bool tick() final;
			void queuePacket(std::shared_ptr<Packet>);
			void chunkReceived(ChunkPosition);
			void interactOn(Modifiers, Hand = Hand::None);
			void interactNextTo(Modifiers, Hand = Hand::None);
			void putInLimbo(EntityPtr, RealmID, const Position &);
			void requestFromLimbo(RealmID);
			void playSound(const Identifier &);

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

			Side getSide() const final { return Side::Client; }

			/** Returns whether the thread could be started. The thread can't be started if the thread is already running. */
			bool startThread();
			void stopThread();

			std::shared_ptr<ClientGame> getSelf() { return std::static_pointer_cast<ClientGame>(shared_from_this()); }
			std::shared_ptr<const ClientGame> getSelf() const { return std::static_pointer_cast<const ClientGame>(shared_from_this()); }

		private:
			sigc::signal<void(const PlayerPtr &)> signal_player_inventory_update_;
			sigc::signal<void(const PlayerPtr &)> signal_player_money_update_;
			sigc::signal<void(const std::shared_ptr<Agent> &, InventoryID)> signal_other_inventory_update_;
			sigc::signal<void(const std::shared_ptr<HasFluids> &)> signal_fluid_update_;
			sigc::signal<void(const std::shared_ptr<HasEnergy> &)> signal_energy_update_;
			std::atomic_bool active{false};
			std::thread tickThread;
			std::optional<Position> lastDragPosition;

			Lockable<std::set<ChunkPosition>> missingChunks;
			MTQueue<std::shared_ptr<Packet>> packetQueue;
			/** Temporarily stores shared pointers to entities that have moved to a realm we're unaware of to prevent destruction. */
			Lockable<std::unordered_map<RealmID, std::unordered_map<EntityPtr, Position>>> entityLimbo;
	};

	using ClientGamePtr = std::shared_ptr<ClientGame>;
}
