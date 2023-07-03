#pragma once

#include "game/Game.h"

namespace Game3 {
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
			void tick() override;
			void queuePacket(std::shared_ptr<Packet>);
			void chunkReceived(ChunkPosition);
			void interactOn();
			void interactNextTo();

			sigc::signal<void(const PlayerPtr &)> signal_player_inventory_update() const { return signal_player_inventory_update_; }
			sigc::signal<void(const PlayerPtr &)> signal_player_money_update() const { return signal_player_money_update_; }
			sigc::signal<void(const std::shared_ptr<HasRealm> &)> signal_other_inventory_update()  const { return signal_other_inventory_update_; }

			Side getSide() const override { return Side::Client; }

		private:
			sigc::signal<void(const PlayerPtr &)> signal_player_inventory_update_;
			sigc::signal<void(const PlayerPtr &)> signal_player_money_update_;
			sigc::signal<void(const std::shared_ptr<HasRealm> &)> signal_other_inventory_update_;

			std::set<ChunkPosition> missingChunks;
			MTQueue<std::shared_ptr<Packet>> packetQueue;
	};
}
