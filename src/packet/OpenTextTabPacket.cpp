#include "Log.h"
#include "entity/ClientPlayer.h"
#include "game/ClientGame.h"
#include "packet/OpenTextTabPacket.h"
#include "ui/MainWindow.h"
#include "ui/tab/InventoryTab.h"
#include "ui/tab/TextTab.h"

namespace Game3 {
	void OpenTextTabPacket::handle(ClientGame &game) {
		MainWindow &window = game.getWindow();

		if (removeOnMove) {
			game.player->queueForMove([&window](const auto &) {
				window.queue([&window] {
					window.inventoryTab->show();
				});
				return true;
			});
		}

		window.queue([&game, &window, name = std::move(name), message = std::move(message), ephemeral = ephemeral] {
			TextTab &tab = *window.textTab;
			tab.name = std::move(name);
			tab.text = std::move(message);
			tab.ephemeral = ephemeral;
			tab.reset(game.getSelf());
			tab.show();
		});
	}
}
