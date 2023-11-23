#include <iostream>

#include "Log.h"
#include "entity/ClientPlayer.h"
#include "game/ClientGame.h"
#include "game/ClientInventory.h"
#include "item/Tool.h"
#include "packet/MoveSlotsPacket.h"
#include "packet/SetHeldItemPacket.h"
#include "ui/MainWindow.h"
#include "ui/gtk/EntryDialog.h"
#include "ui/gtk/NumericEntry.h"
#include "ui/gtk/UITypes.h"
#include "ui/gtk/Util.h"
#include "ui/tab/InventoryTab.h"
#include "ui/module/ExternalInventoryModule.h"
#include "ui/module/Module.h"
#include "util/Util.h"

namespace Game3 {
	InventoryTab::InventoryTab(MainWindow &main_window): Tab(main_window.notebook), mainWindow(main_window) {
		vbox.append(grid);
		scrolled.set_child(vbox);
		scrolled.set_hexpand();
		scrolled.set_vexpand();

		gmenu = Gio::Menu::create();
		gmenu->append("Hold (_Left)", "inventory_popup.hold_left");
		gmenu->append("Hold (_Right)", "inventory_popup.hold_right");
		gmenu->append("_Drop", "inventory_popup.drop");
		gmenu->append("D_iscard", "inventory_popup.discard");

		auto group = Gio::SimpleActionGroup::create();

		group->add_action("hold_left", [this] {
			if (lastGame)
				lastGame->player->send(SetHeldItemPacket(true, lastSlot));
			else
				WARN(__FILE__ << ':' << __LINE__ << ": no lastGame");
		});

		group->add_action("hold_right", [this] {
			if (lastGame)
				lastGame->player->send(SetHeldItemPacket(false, lastSlot));
			else
				WARN(__FILE__ << ':' << __LINE__ << ": no lastGame");
		});

		group->add_action("drop", [this] {
			if (lastGame)
				lastGame->player->getInventory(0)->drop(lastSlot);
			else
				WARN(__FILE__ << ':' << __LINE__ << ": no lastGame");
		});

		group->add_action("discard", [this] {
			if (lastGame)
				lastGame->player->getInventory(0)->discard(lastSlot);
			else
				WARN(__FILE__ << ':' << __LINE__ << ": no lastGame");
		});

		mainWindow.insert_action_group("inventory_popup", group);
		popoverMenu.set_parent(vbox);

		auto source = Gtk::DragSource::create();
		source->set_actions(Gdk::DragAction::MOVE);
		source->signal_prepare().connect([this, source](double x, double y) -> Glib::RefPtr<Gdk::ContentProvider> { // Does capturing `source` cause a memory leak?
			auto *item = grid.pick(x, y);

			if (dynamic_cast<Gtk::Fixed *>(item->get_parent()))
				item = item->get_parent();

			if (auto *label = dynamic_cast<Gtk::Label *>(item)) {
				if (label->get_text().empty())
					return nullptr;
			} else if (!dynamic_cast<Gtk::Fixed *>(item))
				return nullptr;

			Glib::Value<DragSource> value;
			value.init(value.value_type());
			value.set({widgetMap.at(item), std::static_pointer_cast<ClientInventory>(mainWindow.game->player->getInventory(0)), 0});
			return Gdk::ContentProvider::create(value);
		}, false);

		auto target = Gtk::DropTarget::create(Glib::Value<DragSource>::value_type(), Gdk::DragAction::MOVE);
		target->signal_drop().connect([this](const Glib::ValueBase &base, double x, double y) {
			if (base.gobj()->g_type != Glib::Value<DragSource>::value_type())
				return false;

			const auto &value = static_cast<const Glib::Value<DragSource> &>(base);
			auto *destination = grid.pick(x, y);

			if (destination != nullptr && destination != &grid) {
				if (dynamic_cast<Gtk::Fixed *>(destination->get_parent()))
					destination = destination->get_parent();
				const DragSource source = value.get();
				ClientPlayer &player = *mainWindow.game->player;
				player.send(MoveSlotsPacket(source.inventory->getOwner()->getGID(), player.getGID(), source.slot, widgetMap.at(destination), source.index, 0));
			}

			return true;
		}, false);

		grid.add_controller(source);
		grid.add_controller(target);
		grid.set_row_homogeneous();
		grid.set_column_homogeneous();
		grid.set_hexpand();
		vbox.set_hexpand();
		vbox.set_vexpand();
	}

	void InventoryTab::onResize(const std::shared_ptr<ClientGame> &game) {
		if (gridWidth() != lastGridWidth) {
			reset(game);
			auto lock = currentModule.sharedLock();
			if (currentModule)
				currentModule->onResize(grid.get_width());
		}
	}

	void InventoryTab::update(const std::shared_ptr<ClientGame> &game) {
		if (!game || !game->player)
			return;

		lastGame = game;

		mainWindow.queue([this, game] {
			if (const InventoryPtr inventory = game->player->getInventory(0))
				populate(grid, std::static_pointer_cast<ClientInventory>(inventory));

			auto lock = currentModule.trySharedLock();
			if (currentModule)
				currentModule->update();
		});
	}

	void InventoryTab::reset(const std::shared_ptr<ClientGame> &game) {
		if (!game) {
			clear();
			lastGame = nullptr;
			return;
		}

		if (!game->player)
			return;

		lastGame = game;

		mainWindow.queue([this, game] {
			clear();

			if (const InventoryPtr inventory = game->player->getInventory(0))
				populate(grid, std::static_pointer_cast<ClientInventory>(inventory));

			auto lock = currentModule.sharedLock();
			if (currentModule)
				currentModule->reset();
		});
	}

	void InventoryTab::clear() {
		clickGestures.clear();
		widgetMap.clear();
		removeChildren(grid);
		widgetsBySlot.clear();
		widgets.clear();
	}

	void InventoryTab::populate(Gtk::Grid &grid, std::shared_ptr<ClientInventory> inventory) {
		if (!lastGame)
			return;

		std::shared_ptr<ClientGame> last_game = lastGame.copyBase();

		auto &storage = inventory->getStorage();
		const int grid_width = lastGridWidth = gridWidth();
		const int tile_size  = grid.get_width() / (grid.get_width() / TILE_SIZE);
		const bool tooldown = 0.f < last_game->player->tooldown;


		for (Slot slot = 0; slot < inventory->slotCount; ++slot) {
			const int row    = slot / grid_width;
			const int column = slot % grid_width;
			std::unique_ptr<Gtk::Widget> widget_ptr;

			if (storage.contains(slot)) {
				auto &stack = storage.at(slot);
				Glib::ustring tooltip_text = stack.item->getTooltip(stack);
				if (stack.count != 1)
					tooltip_text += " \u00d7 " + std::to_string(stack.count);
				if (stack.hasDurability())
					tooltip_text += "\n(" + std::to_string(stack.data.at("durability").at(0).get<Durability>()) + "/" + std::to_string(stack.data.at("durability").at(1).get<Durability>()) + ")";
				auto fixed_ptr = std::make_unique<Gtk::Fixed>();
				auto image_ptr = std::make_unique<Gtk::Image>(inventory->getImage(*last_game, slot));
				auto label_ptr = std::make_unique<Gtk::Label>(std::to_string(stack.count));
				label_ptr->set_xalign(1.f);
				label_ptr->set_yalign(1.f);
				auto &fixed = *fixed_ptr;
				if (stack.hasDurability()) {
					auto progress_ptr = std::make_unique<Gtk::ProgressBar>();
					progress_ptr->set_fraction(stack.getDurabilityFraction());
					progress_ptr->add_css_class("item-durability");
					progress_ptr->set_size_request(tile_size - TILE_MAGIC, -1);
					fixed.put(*progress_ptr, 0, 0);
					widgets.push_back(std::move(progress_ptr));
				}
				fixed.put(*image_ptr, 0, 0);
				fixed.put(*label_ptr, 0, 0);
				fixed.set_tooltip_text(tooltip_text);
				widget_ptr = std::move(fixed_ptr);
				image_ptr->set_size_request(tile_size - TILE_MAGIC, tile_size - TILE_MAGIC);
				if (tooldown && dynamic_cast<Tool *>(stack.item.get()))
					image_ptr->set_opacity(0.5);
				label_ptr->set_size_request(tile_size - TILE_MAGIC, tile_size - TILE_MAGIC);
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
			if (slot == inventory->activeSlot)
				widget_ptr->add_css_class("active-slot");

			Gtk::Widget *old_widget = nullptr;
			if (auto iter = widgetsBySlot.find(slot); iter != widgetsBySlot.end()) {
				old_widget = iter->second;
				widgetMap.erase(iter->second);
			}
			widgetsBySlot[slot] = widget_ptr.get();

			auto left_click = Gtk::GestureClick::create();
			left_click->set_button(1);
			left_click->signal_released().connect([this, last_game, slot, widget = widget_ptr.get()](int n, double x, double y) {
				const auto mods = clickGestures[widget].first->get_current_event_state();
				leftClick(last_game, widget, n, slot, Modifiers{mods}, x, y);
			});

			auto right_click = Gtk::GestureClick::create();
			right_click->set_button(3);
			right_click->signal_pressed().connect([this, last_game, slot, widget = widget_ptr.get()](int n, double x, double y) {
				const auto mods = clickGestures[widget].second->get_current_event_state();
				rightClick(last_game, widget, n, slot, Modifiers{mods}, x, y);
			});

			widget_ptr->add_controller(left_click);
			widget_ptr->add_controller(right_click);

			clickGestures[widget_ptr.get()] = {std::move(left_click), std::move(right_click)};
			widgetMap[widget_ptr.get()] = slot;

			if (old_widget != nullptr)
				grid.remove(*old_widget);
			grid.attach(*widget_ptr, column, row);
			widgets.push_back(std::move(widget_ptr));
		}
	}

	void InventoryTab::setModule(std::unique_ptr<Module> &&module_) {
		assert(module_);
		removeModule();
		auto lock = currentModule.uniqueLock();
		currentModule.std::unique_ptr<Module>::operator=(std::move(module_));
		vbox.append(currentModule->getWidget());
		currentModule->onResize(grid.get_width());
		currentModule->reset();
	}

	Module & InventoryTab::getModule() const {
		assert(currentModule);
		return *currentModule;
	}

	Module * InventoryTab::getModule(std::shared_lock<DefaultMutex> &lock) {
		if (currentModule)
			lock = currentModule.sharedLock();
		return currentModule.get();
	}

	Module * InventoryTab::getModule(std::unique_lock<DefaultMutex> &lock) {
		if (currentModule)
			lock = currentModule.uniqueLock();
		return currentModule.get();
	}

	void InventoryTab::removeModule() {
		auto lock = currentModule.uniqueLock();
		if (currentModule) {
			vbox.remove(currentModule->getWidget());
			currentModule.reset();
		}
	}

	GlobalID InventoryTab::getExternalGID() const {
		throw std::logic_error("InventoryTab::getExternalGID() needs to be replaced");
	}

	int InventoryTab::gridWidth() const {
		return scrolled.get_width() / (TILE_SIZE + 2 * TILE_MARGIN);
	}

	void InventoryTab::leftClick(const std::shared_ptr<ClientGame> &game, Gtk::Widget *, int, Slot slot, Modifiers modifiers, double, double) {
		mainWindow.onBlur();

		if (modifiers.onlyShift()) {
			shiftClick(game, slot);
		} else {
			game->player->getInventory(0)->setActive(slot, false);
			updatePlayerClasses(game);
		}
	}

	void InventoryTab::rightClick(const std::shared_ptr<ClientGame> &game, Gtk::Widget *widget, int, Slot slot, Modifiers, double x, double y) {
		mainWindow.onBlur();

		if (!game->player->getInventory(0)->contains(slot))
			return;

		const auto allocation = widget->get_allocation();
		x += allocation.get_x();
		y += allocation.get_y();

		popoverMenu.set_has_arrow(true);
		popoverMenu.set_pointing_to({int(x), int(y), 1, 1});
		popoverMenu.set_menu_model(gmenu);
		lastGame = game;
		lastSlot = slot;
		popoverMenu.popup();
	}

	void InventoryTab::shiftClick(const std::shared_ptr<ClientGame> &game, Slot slot) {
		if (!game)
			return;

		InventoryPtr inventory = game->player->getInventory(0);
		if (!inventory || !inventory->contains(slot))
			return;

		std::unique_lock<DefaultMutex> lock;
		Module *module_ = getModule(lock);
		if (!module_)
			return;

		if (module_->handleShiftClick(inventory, slot))
			return;

		auto *external = dynamic_cast<ExternalInventoryModule *>(module_);
		if (!external)
			return;

		InventoryPtr external_inventory = external->getInventory();
		if (!external_inventory)
			return;

		AgentPtr owner = external_inventory->weakOwner.lock();
		if (!owner)
			return;

		game->player->send(MoveSlotsPacket(game->player->getGID(), owner->getGID(), slot, -1, 0, external_inventory->index));
	}

	void InventoryTab::updatePlayerClasses(const std::shared_ptr<ClientGame> &game) {
		const Slot active_slot = game->player->getInventory(0)->activeSlot;

		if (widgetsBySlot.contains(active_slot))
			widgetsBySlot.at(active_slot)->add_css_class("active-slot");

		for (auto &[slot, widget]: widgetsBySlot)
			if (slot != active_slot)
				widget->remove_css_class("active-slot");
	}
}
