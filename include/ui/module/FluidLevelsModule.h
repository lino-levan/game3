#pragma once

#include "types/Types.h"
#include "ui/module/Module.h"

#include <any>
#include <memory>
#include <vector>

namespace Game3 {
	class Agent;
	class HasFluids;
	class InventoryTab;

	class FluidLevelsModule: public Module {
		public:
			static Identifier ID() { return {"base", "module/fluid_levels"}; }

			FluidLevelsModule(std::shared_ptr<ClientGame>, const std::any &);

			Identifier getID() const final { return ID(); }
			Gtk::Widget & getWidget() final;
			void reset()  final;
			void update() final;
			std::optional<Buffer> handleMessage(const std::shared_ptr<Agent> &source, const std::string &name, std::any &data) final;

			void updateIf(const std::shared_ptr<HasFluids> &);

		private:
			std::shared_ptr<ClientGame> game;
			std::shared_ptr<HasFluids> fluidHaver;
			std::vector<std::unique_ptr<Gtk::Widget>> widgets;
			Gtk::Box vbox{Gtk::Orientation::VERTICAL};

			void populate();
	};
}
