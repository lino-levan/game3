#pragma once

#include "types/DirectedPlace.h"
#include "types/Types.h"
#include "ui/gtk/Util.h"
#include "ui/module/Module.h"

#include <any>
#include <memory>
#include <optional>
#include <vector>

namespace Game3 {
	class Agent;
	class HasFluids;
	class InventoryTab;
	class ItemFilter;

	class ItemFilterModule: public Module {
		public:
			static Identifier ID() { return {"base", "module/item_filters"}; }

			ItemFilterModule(std::shared_ptr<ClientGame>, const std::any &);

			Identifier getID() const final { return ID(); }
			Gtk::Widget & getWidget() final;
			void reset()  final;
			void update() final;
			std::optional<Buffer> handleMessage(const std::shared_ptr<Agent> &source, const std::string &name, std::any &data) final;

		private:
			std::shared_ptr<ClientGame> game;
			DirectedPlace place;
			std::vector<std::unique_ptr<Gtk::Widget>> widgets;
			Gtk::Box vbox{Gtk::Orientation::VERTICAL};
			std::shared_ptr<ItemFilter> filter;
			Gtk::Fixed fixed;

			Gtk::Label modeLabel{"Whitelist"};
			Gtk::Label strictLabel{"Strict"};
			Gtk::Switch modeSwitch;
			Gtk::Switch strictSwitch;
			Gtk::Box modeHbox{Gtk::Orientation::HORIZONTAL};
			Gtk::Box strictHbox{Gtk::Orientation::HORIZONTAL};

			Gtk::Box switchesHbox{Gtk::Orientation::HORIZONTAL};

			void setMode(bool allow);
			void setStrict(bool strict);
			void upload();
			void populate();
	};
}
