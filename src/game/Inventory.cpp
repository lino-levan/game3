#include <cassert>

#include "Log.h"
#include "entity/Entity.h"
#include "entity/ItemEntity.h"
#include "entity/Player.h"
#include "entity/ServerPlayer.h"
#include "game/Agent.h"
#include "game/ClientGame.h"
#include "game/ClientInventory.h"
#include "game/Inventory.h"
#include "game/ServerInventory.h"
#include "net/Buffer.h"
#include "packet/InventoryPacket.h"
#include "packet/SetActiveSlotPacket.h"
#include "packet/DropItemPacket.h"
#include "realm/Realm.h"
#include "recipe/CraftingRecipe.h"
#include "util/Util.h"

namespace Game3 {
	Inventory::Inventory() = default;

	Inventory::Inventory(std::shared_ptr<Agent> owner, Slot active_slot, InventoryID index_):
		weakOwner(owner), activeSlot(active_slot), index(index_) {}

	Inventory & Inventory::operator=(const Inventory &other) {
		if (this == &other)
			return *this;

		auto this_lock = uniqueLock();
		auto other_lock = other.sharedLock();
		weakOwner = other.weakOwner;
		setSlotCount(other.getSlotCount());
		activeSlot = other.activeSlot.load();
		index = other.index.load();
		suppressInventoryNotifications = other.suppressInventoryNotifications.load();
		onSwap = other.onSwap;
		onMove = other.onMove;
		return *this;
	}

	Inventory & Inventory::operator=(Inventory &&other) {
		if (this == &other)
			return *this;

		auto this_lock = uniqueLock();
		auto other_lock = other.uniqueLock();
		weakOwner = std::move(other.weakOwner);
		setSlotCount(other.getSlotCount());
		other.setSlotCount(0);
		activeSlot = other.activeSlot.exchange(0);
		index = other.index.load();
		suppressInventoryNotifications = other.suppressInventoryNotifications.exchange(false);
		onSwap = std::move(other.onSwap);
		onMove = std::move(other.onMove);
		return *this;
	}

	bool Inventory::operator==(const Inventory &other) const {
		if (this == &other)
			return true;

		auto owner = weakOwner.lock();
		return owner && index != InventoryID(-1) && owner == other.weakOwner.lock() && index == other.index;
	}

	bool Inventory::decrease(ItemStack &stack, Slot slot, ItemCount amount, bool do_lock) {
		auto lock = do_lock? uniqueLock() : std::unique_lock<DefaultMutex>{};

		if (stack.count < amount)
			throw std::runtime_error("Can't decrease stack count " + std::to_string(stack.count) + " by " + std::to_string(amount));

		stack.count -= amount;

		bool erased = false;
		if (stack.count == 0) {
			erase(slot);
			erased = true;
		}

		notifyOwner();
		return erased;
	}

	void Inventory::erase() {
		erase(activeSlot);
	}

	std::shared_ptr<Agent> Inventory::getOwner() const {
		if (auto owner = weakOwner.lock())
			return owner;
		throw std::runtime_error("Couldn't lock inventory owner");
	}

	void Inventory::prevSlot() {
		if (0 < activeSlot)
			setActive(activeSlot - 1, false);
	}

	void Inventory::nextSlot() {
		if (activeSlot < getSlotCount() - 1)
			setActive(activeSlot + 1, false);
	}

	ItemCount Inventory::craftable(const CraftingRecipe &recipe) const {
		ItemCount out = std::numeric_limits<ItemCount>::max();

		for (const auto &input: recipe.input) {
			if (input.is<ItemStack>()) {
				const ItemStack &stack = input.get<ItemStack>();
				out = std::min(out, count(stack) / stack.count);
			} else {
				const auto &[attribute, attribute_count] = input.get<AttributeRequirement>();
				out = std::min(out, countAttribute(attribute) / attribute_count);
			}
		}

		return out;
	}

	std::shared_ptr<Inventory> Inventory::create(Side side, std::shared_ptr<Agent> owner, Slot slot_count, InventoryID index, Slot active_slot, std::map<Slot, ItemStack> storage) {
		if (side == Side::Server)
			return std::make_shared<ServerInventory>(owner, slot_count, active_slot, index, std::move(storage));
		if (side == Side::Client)
			return std::make_shared<ClientInventory>(owner, slot_count, active_slot, index, std::move(storage));
		throw std::invalid_argument("Can't create inventory for side " + std::to_string(static_cast<int>(side)));
	}

	std::shared_ptr<Inventory> Inventory::create(std::shared_ptr<Agent> owner, Slot slot_count, InventoryID index, Slot active_slot, std::map<Slot, ItemStack> storage) {
		return create(owner->getSide(), owner, slot_count, active_slot, index, std::move(storage));
	}
}
