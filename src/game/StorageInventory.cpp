#include "game/StorageInventory.h"
#include "recipe/CraftingRecipe.h"

namespace Game3 {
	StorageInventory::StorageInventory(std::shared_ptr<Agent> owner, Slot slot_count, Slot active_slot, Storage storage_):
		Inventory(std::move(owner), slot_count, active_slot), storage(std::move(storage_)) {}

	StorageInventory::StorageInventory(const StorageInventory &other):
	Inventory(other) {
		auto lock = other.sharedLock();
		storage = other.storage;
		onSwap = other.onSwap;
	}

	StorageInventory::StorageInventory(StorageInventory &&other):
	Inventory(std::forward<Inventory>(other)) {
		auto lock = other.uniqueLock();
		storage = std::move(other.storage);
		onSwap = std::move(other.onSwap);
	}

	StorageInventory & StorageInventory::operator=(const StorageInventory &other) {
		if (this == &other)
			return *this;

		Inventory::operator=(other);
		auto this_lock = uniqueLock();
		auto other_lock = other.sharedLock();
		storage = other.storage;
		onSwap = other.onSwap;
		return *this;
	}

	StorageInventory & StorageInventory::operator=(StorageInventory &&other) {
		if (this == &other)
			return *this;

		Inventory::operator=(std::move(other));
		auto this_lock = uniqueLock();
		auto other_lock = other.uniqueLock();
		storage = std::move(other.storage);
		onSwap = std::move(other.onSwap);
		return *this;
	}

	ItemStack * StorageInventory::operator[](Slot slot) {
		if (auto iter = storage.find(slot); iter != storage.end())
			return &iter->second;
		return nullptr;
	}

	const ItemStack * StorageInventory::operator[](Slot slot) const {
		if (auto iter = storage.find(slot); iter != storage.end())
			return &iter->second;
		return nullptr;
	}

	void StorageInventory::iterate(const std::function<bool(const ItemStack &, Slot)> &function) const {
		for (Slot slot = 0; slot < slotCount; ++slot)
			if (auto iter = storage.find(slot); iter != storage.end())
				if (function(iter->second, slot))
					return;
	}

	void StorageInventory::iterate(const std::function<bool(ItemStack &, Slot)> &function) {
		for (Slot slot = 0; slot < slotCount; ++slot)
			if (auto iter = storage.find(slot); iter != storage.end())
				if (function(iter->second, slot))
					return;
	}

	ItemStack * StorageInventory::firstItem(Slot *slot_out) {
		if (storage.empty()) {
			if (slot_out != nullptr)
				*slot_out = -1;
			return nullptr;
		}

		auto &[slot, stack] = *storage.begin();
		if (slot_out)
			*slot_out = slot;
		return &stack;
	}

	ItemStack * StorageInventory::firstItem(Slot *slot_out, const std::function<bool(const ItemStack &, Slot)> &predicate) {
		for (auto &[slot, stack]: storage) {
			if (predicate(stack, slot)) {
				if (slot_out)
					*slot_out = slot;
				return &stack;
			}
		}

		if (slot_out)
			*slot_out = -1;
		return nullptr;
	}

	bool StorageInventory::canInsert(const ItemStack &stack, const std::function<bool(Slot)> &predicate) const {
		ssize_t remaining = stack.count;

		for (const auto &[slot, stored]: storage) {
			if (!predicate(slot) || !stored.canMerge(stack))
				continue;
			const ssize_t storable = ssize_t(stored.item->maxCount) - ssize_t(stored.count);
			if (0 < storable) {
				const ItemCount to_store = std::min(ItemCount(remaining), ItemCount(storable));
				remaining -= to_store;
				if (remaining <= 0)
					break;
			}
		}

		if (0 < remaining) {
			for (Slot slot = 0; slot < slotCount; ++slot) {
				if (storage.contains(slot))
					continue;
				remaining -= std::min(ItemCount(remaining), stack.item->maxCount);
				if (remaining <= 0)
					break;
			}
		}

		if (remaining < 0)
			throw std::logic_error("How'd we end up with " + std::to_string(remaining) + " items remaining?");

		return remaining == 0;
	}

	bool StorageInventory::canInsert(const ItemStack &stack, Slot slot) const {
		auto iter = storage.find(slot);

		if (iter == storage.end())
			return stack.count <= stack.item->maxCount;

		const ItemStack &stored = iter->second;

		if (!stored.canMerge(stack))
			return false;

		ssize_t remaining = stack.count;
		const ssize_t storable = ssize_t(stored.item->maxCount) - ssize_t(stored.count);

		if (0 < storable) {
			const ItemCount to_store = std::min<ItemCount>(remaining, storable);
			remaining -= to_store;
			assert(0 <= remaining);
			return remaining == 0;
		}

		return false;
	}

	bool StorageInventory::canExtract(Slot slot) const {
		return storage.contains(slot);
	}

	ItemCount StorageInventory::insertable(const ItemStack &stack, Slot slot) const {
		auto iter = storage.find(slot);

		if (iter == storage.end())
			return stack.count;

		const ItemStack &stored = iter->second;

		if (!stored.canMerge(stack))
			return 0;

		return stored.item->maxCount - stack.count;
	}

	ItemCount StorageInventory::count(const ItemID &id) const {
		if (id.getPath() == "attribute")
			return countAttribute(id);

		ItemCount out = 0;

		for (const auto &[slot, stack]: storage)
			if (stack.item->identifier == id)
				out += stack.count;

		return out;
	}

	ItemCount StorageInventory::count(const Item &item) const {
		ItemCount out = 0;

		for (const auto &[slot, stack]: storage)
			if (stack.item->identifier == item.identifier)
				out += stack.count;

		return out;
	}

	ItemCount StorageInventory::count(const ItemStack &stack) const {
		ItemCount out = 0;

		for (const auto &[slot, stored_stack]: storage)
			if (stack.canMerge(stored_stack))
				out += stored_stack.count;

		return out;
	}

	ItemCount StorageInventory::count(const ItemStack &stack, const std::function<bool(Slot)> &predicate) const {
		ItemCount out = 0;

		for (const auto &[slot, stored_stack]: storage)
			if (predicate(slot) && stack.canMerge(stored_stack))
				out += stored_stack.count;

		return out;
	}

	ItemCount StorageInventory::countAttribute(const Identifier &attribute) const {
		ItemCount out = 0;

		for (const auto &[slot, stack]: storage)
			if (stack.hasAttribute(attribute))
				out += stack.count;

		return out;
	}

	bool StorageInventory::hasSlot(Slot slot) const {
		return 0 <= slot && slot < slotCount;
	}

	bool StorageInventory::empty() const {
		return storage.empty();
	}

	ItemStack & StorageInventory::front() {
		if (storage.empty())
			throw std::out_of_range("Inventory empty");
		return storage.begin()->second;
	}

	const ItemStack & StorageInventory::front() const {
		if (storage.empty())
			throw std::out_of_range("Inventory empty");
		return storage.begin()->second;
	}

	bool StorageInventory::contains(Slot slot) const {
		return storage.contains(slot);
	}

	std::optional<Slot> StorageInventory::find(const ItemID &id, const ConstPredicate &predicate) const {
		for (const auto &[slot, stack]: storage)
			if (predicate(stack, slot) && stack.item->identifier == id)
				return slot;
		return std::nullopt;
	}

	std::optional<Slot> StorageInventory::findAttribute(const Identifier &attribute, const ConstPredicate &predicate) const {
		for (const auto &[slot, stack]: storage)
			if (predicate(stack, slot) && stack.item->attributes.contains(attribute))
				return slot;
		return std::nullopt;
	}

	ItemStack * StorageInventory::getActive() {
		if (auto iter = storage.find(activeSlot); iter != storage.end())
			return &iter->second;
		return nullptr;
	}

	const ItemStack * StorageInventory::getActive() const {
		if (auto iter = storage.find(activeSlot); iter != storage.end())
			return &iter->second;
		return nullptr;
	}

	bool StorageInventory::contains(const ItemStack &needle, const ConstPredicate &predicate) const {
		ItemCount remaining = needle.count;
		for (const auto &[slot, stack]: storage) {
			if (!predicate(stack, slot) || !needle.canMerge(stack))
				continue;
			if (remaining <= stack.count)
				return true;
			remaining -= stack.count;
		}

		return false;
	}

	void StorageInventory::prevSlot() {
		if (0 < activeSlot)
			setActive(activeSlot - 1, false);
	}

	void StorageInventory::nextSlot() {
		if (activeSlot < slotCount - 1)
			setActive(activeSlot + 1, false);
	}

	void StorageInventory::compact() {
		auto lock = storage.uniqueLock();

		for (auto iter = storage.begin(); iter != storage.end();) {
			if (iter->second.count == 0)
				storage.erase(iter++);
			else
				++iter;
		}
	}
}
