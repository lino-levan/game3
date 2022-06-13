#include <iostream>

#include "game/Game.h"
#include "game/Inventory.h"
#include "ui/MainWindow.h"
#include "ui/gtk/EntryDialog.h"
#include "ui/gtk/NumericEntry.h"
#include "ui/gtk/Util.h"
#include "ui/tab/InventoryTab.h"
#include "util/Util.h"

namespace Game3 {
	InventoryTab::InventoryTab(MainWindow &main_window): Tab(main_window.notebook), mainWindow(main_window) {
		vbox.append(playerGrid);
		vbox.append(hbox);
		vbox.append(externalGrid);
		hbox.append(externalLabel);
		externalLabel.set_hexpand(true);
		scrolled.set_child(vbox);
		scrolled.set_vexpand(true);
		auto gmenu = Gio::Menu::create();
		gmenu->append("_Drop", "inventory_popup.drop");
		gmenu->append("D_iscard", "inventory_popup.discard");
		popoverMenu.set_menu_model(gmenu);

		auto group = Gio::SimpleActionGroup::create();
		group->add_action("drop", [this] {
			(lastExternal? externalInventory : lastGame->player->inventory)->drop(lastSlot);
		});
		group->add_action("discard", [this] {
			(lastExternal? externalInventory : lastGame->player->inventory)->erase(lastSlot);
		});

		mainWindow.insert_action_group("inventory_popup", group);
		popoverMenu.set_parent(mainWindow); // TODO: fix this silliness

		for (bool external: {false, true}) {
			auto source = Gtk::DragSource::create();
			source->set_actions(Gdk::DragAction::MOVE);
			source->signal_prepare().connect([this, source, external](double x, double y) -> Glib::RefPtr<Gdk::ContentProvider> { // Does capturing `source` cause a memory leak?
				auto *item = (external? externalGrid : playerGrid).pick(x, y);
				scrolled.set_data("dragged-item", nullptr);

				if (dynamic_cast<Gtk::Fixed *>(item->get_parent()))
					item = item->get_parent();

				if (auto *label = dynamic_cast<Gtk::Label *>(item)) {
					if (label->get_text().empty())
						return nullptr;
				} else if (!dynamic_cast<Gtk::Fixed *>(item))
					return nullptr;

				scrolled.set_data("dragged-item", item);
				Glib::ValueBase base;
				base.init(GTK_TYPE_WIDGET);
				return Gdk::ContentProvider::create(base);
			}, false);
			(external? externalGrid : playerGrid).add_controller(source);

			auto target = Gtk::DropTarget::create(GTK_TYPE_WIDGET, Gdk::DragAction::MOVE);
			target->signal_drop().connect([this, external](const Glib::ValueBase &, double x, double y) {
				auto &grid = external? externalGrid : playerGrid;
				auto *destination = grid.pick(x, y);

				if (destination != nullptr && destination != &grid) {
					if (dynamic_cast<Gtk::Fixed *>(destination->get_parent()))
						destination = destination->get_parent();

					auto &dragged = getDraggedItem();
					const Slot source_slot      = reinterpret_cast<intptr_t>(dragged.get_data("slot"));
					const Slot destination_slot = reinterpret_cast<intptr_t>(destination->get_data("slot"));
					const bool from_external    = reinterpret_cast<intptr_t>(dragged.get_data("external"));
					const bool to_external      = reinterpret_cast<intptr_t>(destination->get_data("external"));
					auto &player_inventory = *mainWindow.game->player->inventory;

					if (from_external) {
						if (to_external) {
							externalInventory->swap(source_slot, destination_slot);
						} else {
							const ItemStack *source = (*externalInventory)[source_slot];
							if (!source) {
								std::cerr << "Warning: slot " << source_slot << " not present in external inventory\n";
								return true;
							}

							if (player_inventory.canStore(*source)) {
								player_inventory.add(*source, destination_slot);
								externalInventory->erase(source_slot);
							}
						}
					} else {
						if (to_external) {
							const ItemStack *source = player_inventory[source_slot];
							if (!source) {
								std::cerr << "Warning: slot " << source_slot << " not present in player inventory\n";
								return true;
							}

							if (externalInventory->canStore(*source)) {
								externalInventory->add(*source, destination_slot);
								player_inventory.erase(source_slot);
							}
						} else
							player_inventory.swap(source_slot, destination_slot);
					}
				}

				return true;
			}, false);
			(external? externalGrid : playerGrid).add_controller(target);
		}
	}

	void InventoryTab::onResize(const std::shared_ptr<Game> &game) {
		if (gridWidth() != lastGridWidth)
			reset(game);
	}

	void InventoryTab::update(const std::shared_ptr<Game> &) {

	}

	void InventoryTab::reset(const std::shared_ptr<Game> &game) {
		if (!game)
			return;

		removeChildren(playerGrid);
		removeChildren(externalGrid);
		if (!externalName.empty()) {
			externalLabel.set_text(externalName);
			externalLabel.show();
		}

		playerWidgets.clear();
		externalWidgets.clear();

		const int grid_width = lastGridWidth = gridWidth();
		const int tile_size  = playerGrid.get_width() / (playerGrid.get_width() / TILE_SIZE);

		auto populate = [&](Gtk::Grid &grid, Inventory &inventory, bool external) {
			auto &storage = inventory.getStorage();
			auto &widgets = external? externalWidgets : playerWidgets;

			for (Slot slot = 0; slot < inventory.slotCount; ++slot) {
				const int row    = slot / grid_width;
				const int column = slot % grid_width;
				std::unique_ptr<Gtk::Widget> widget_ptr;

				if (storage.contains(slot)) {
					auto &stack = storage.at(slot);
					Glib::ustring label_text = stack.item->name;
					if (stack.count != 1)
						label_text += " \u00d7 " + std::to_string(stack.count);
					auto fixed_ptr = std::make_unique<Gtk::Fixed>();
					auto image_ptr = std::make_unique<Gtk::Image>(inventory.getImage(slot));
					auto label_ptr = std::make_unique<Gtk::Label>(std::to_string(stack.count));
					label_ptr->set_xalign(1.f);
					label_ptr->set_yalign(1.f);
					auto &fixed = *fixed_ptr;
					fixed.put(*image_ptr, 0, 0);
					fixed.put(*label_ptr, 0, 0);
					fixed.set_tooltip_text(label_text);
					auto left_click = Gtk::GestureClick::create();
					auto right_click = Gtk::GestureClick::create();
					left_click->set_button(1);
					right_click->set_button(3);
					left_click ->signal_pressed().connect([this, game, slot, external, &fixed](int n, double x, double y) { leftClick (game, &fixed, n, slot, external, x, y); });
					right_click->signal_pressed().connect([this, game, slot, external, &fixed](int n, double x, double y) { rightClick(game, &fixed, n, slot, external, x, y); });
					fixed.add_controller(left_click);
					fixed.add_controller(right_click);
					widget_ptr = std::move(fixed_ptr);
					image_ptr->set_size_request(tile_size - InventoryTab::TILE_MAGIC, tile_size - InventoryTab::TILE_MAGIC);
					label_ptr->set_size_request(tile_size - InventoryTab::TILE_MAGIC, tile_size - InventoryTab::TILE_MAGIC);
					widgets.push_back(std::move(image_ptr));
					widgets.push_back(std::move(label_ptr));
				} else
					widget_ptr = std::make_unique<Gtk::Label>("");

				if (auto label = dynamic_cast<Gtk::Label *>(widget_ptr.get())) {
					label->set_wrap(true);
					label->set_wrap_mode(Pango::WrapMode::CHAR);
				}

				widget_ptr->set_size_request(tile_size, tile_size);
				widget_ptr->add_css_class("item-slot");
				widget_ptr->set_data("slot", reinterpret_cast<void *>(slot));
				widget_ptr->set_data("external", reinterpret_cast<void *>(external));
				grid.attach(*widget_ptr, column, row);
				widgets.push_back(std::move(widget_ptr));
			}
		};

		if (game->player->inventory)
			populate(playerGrid, *game->player->inventory, false);

		if (externalInventory)
			populate(externalGrid, *externalInventory, true);
	}

	void InventoryTab::setExternalInventory(const Glib::ustring &name, const std::shared_ptr<Inventory> &inventory) {
		externalInventory = inventory;
		externalName = name;
		if (inventory)
			if (auto owner = inventory->owner.lock())
				reset(owner->getRealm()->getGame().shared_from_this());
	}

	void InventoryTab::resetExternalInventory() {
		removeChildren(externalGrid);
		externalLabel.hide();
		externalLabel.set_text("");
		externalWidgets.clear();
		externalInventory.reset();
		externalName.clear();
	}

	int InventoryTab::gridWidth() const {
		return scrolled.get_width() / (TILE_SIZE + 2 * TILE_MARGIN);
	}

	void InventoryTab::leftClick(const std::shared_ptr<Game> &, Gtk::Widget *, int, Slot, bool, double, double) {
		
	}

	void InventoryTab::rightClick(const std::shared_ptr<Game> &game, Gtk::Widget *widget, int, Slot slot, bool external, double x, double y) {
		do {
			const auto allocation = widget->get_allocation();
			x += allocation.get_x();
			y += allocation.get_y();
			widget = widget->get_parent();
		} while (widget);

		popoverMenu.set_has_arrow(true);
		popoverMenu.set_pointing_to({int(x), int(y), 1, 1});
		lastGame = game;
		lastSlot = slot;
		lastExternal = external;
		popoverMenu.popup();
	}

	Gtk::Widget & InventoryTab::getDraggedItem() {
		return *reinterpret_cast<Gtk::Widget *>(scrolled.get_data("dragged-item"));
	}
}
