#pragma once

#include "types/Types.h"
#include "ui/Modifiers.h"
#include "ui/module/Module.h"

#include <any>
#include <memory>
#include <vector>

namespace Game3 {
	class Agent;
	class ClientInventory;
	class InventoryTab;

	class ExternalInventoryModule: public Module {
		public:
			struct Argument {
				AgentPtr agent;
				InventoryID index;
			};

			static Identifier ID() { return {"base", "module/external_inventory"}; }

			ExternalInventoryModule(std::shared_ptr<ClientGame>, const std::any &);
			ExternalInventoryModule(std::shared_ptr<ClientGame>, std::shared_ptr<ClientInventory>, InventoryID);

			Identifier getID() const final { return ID(); }
			Gtk::Widget & getWidget() final;
			void reset()  final;
			void update() final;
			void onResize(int) final;
			std::optional<Buffer> handleMessage(const std::shared_ptr<Agent> &source, const std::string &name, std::any &data) final;

			inline auto getInventory() const { return inventory; }
			void setInventory(std::shared_ptr<ClientInventory>) override;

			inline auto getInventoryIndex() const { return inventoryIndex; }

		private:
			std::shared_ptr<ClientGame> game;
			std::shared_ptr<ClientInventory> inventory;
			InventoryID inventoryIndex;
			Glib::RefPtr<Gio::Menu> gmenu;
			Glib::RefPtr<Gtk::DragSource> source;
			Glib::ustring name;
			std::unordered_map<Gtk::Widget *, Slot> widgetMap;
			std::unordered_map<Gtk::Widget *, std::pair<Glib::RefPtr<Gtk::GestureClick>, Glib::RefPtr<Gtk::GestureClick>>> clickGestures;
			std::unordered_map<Slot, Gtk::Widget *> widgetsBySlot;
			std::vector<std::unique_ptr<Gtk::Widget>> widgets;
			Gtk::Box vbox{Gtk::Orientation::VERTICAL};
			Gtk::Box hbox{Gtk::Orientation::HORIZONTAL};
			Gtk::PopoverMenu popoverMenu;
			Gtk::Grid grid;
			Gtk::Label label;
			Slot lastSlot = -1;
			int tabWidth = 0;

			static std::shared_ptr<ClientInventory> getInventory(const std::any &);
			static InventoryID getInventoryIndex(const std::any &);
			int gridWidth() const;
			void populate();
			void leftClick(Gtk::Widget *, int click_count, Slot, Modifiers, double x, double y);
			void rightClick(Gtk::Widget *, int click_count, Slot, Modifiers, double x, double y);
	};
}
