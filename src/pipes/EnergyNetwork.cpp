#include "Log.h"
#include "game/EnergyContainer.h"
#include "pipes/EnergyNetwork.h"
#include "realm/Realm.h"
#include "tileentity/EnergeticTileEntity.h"

namespace Game3 {
	EnergyNetwork::EnergyNetwork(size_t id_, const std::shared_ptr<Realm> &realm):
		PipeNetwork(id_, realm), HasEnergy(CAPACITY, 0) {}

	void EnergyNetwork::tick(Tick tick_id) {
		if (!canTick(tick_id))
			return;

		PipeNetwork::tick(tick_id);

		RealmPtr realm = weakRealm.lock();
		if (!realm || insertions.empty())
			return;

		EnergyAmount &energy = energyContainer->energy;
		auto energy_lock = energyContainer->uniqueLock();

		if (0 < energy) {
			energy = distribute(energy);
			if (0 < energy)
				return;
		}

		const EnergyAmount capacity = getEnergyCapacity();
		assert(energy <= capacity);

		{
			std::vector<PairSet::iterator> to_erase;
			auto extractions_lock = extractions.uniqueLock();

			for (auto iter = extractions.begin(); iter != extractions.end(); ++iter) {
				const auto [position, direction] = *iter;
				auto energetic = std::dynamic_pointer_cast<EnergeticTileEntity>(realm->tileEntityAt(position));
				if (!energetic) {
					to_erase.push_back(iter);
					continue;
				}

				energy += energetic->extractEnergy(direction, true, capacity - energy);
				if (capacity <= energy)
					break;
			}

			for (const auto &iter: to_erase)
				extractions.erase(iter);
		}

		energy = distribute(energy);
	}

	EnergyAmount EnergyNetwork::distribute(EnergyAmount amount) {
		if (amount == 0)
			return 0;

		RealmPtr realm = weakRealm.lock();
		if (!realm)
			return amount;

		std::vector<std::pair<std::shared_ptr<EnergeticTileEntity>, Direction>> accepting_insertions;

		{
			auto insertions_lock = insertions.uniqueLock();
			accepting_insertions.resize(insertions.size());

			std::erase_if(insertions, [&](const std::pair<Position, Direction> &pair) {
				const auto [position, direction] = pair;
				auto energetic = std::dynamic_pointer_cast<EnergeticTileEntity>(realm->tileEntityAt(position));
				if (!energetic)
					return true;

				if (energetic->canInsertEnergy(amount, direction))
					accepting_insertions.emplace_back(energetic, direction);

				return false;
			});
		}

		if (accepting_insertions.empty())
			return amount;

		size_t insertions_remaining = accepting_insertions.size();

		for (const auto &[insertion, direction]: accepting_insertions) {
			EnergyAmount to_distribute = amount / insertions_remaining;

			if (insertions_remaining == 1) {
				const EnergyAmount remainder = amount % insertions_remaining;
				to_distribute += remainder;
			}

			const EnergyAmount leftover = insertion->addEnergy(to_distribute, direction);
			const EnergyAmount distributed = to_distribute - leftover;
			amount -= distributed;

			--insertions_remaining;
		}

		return amount;
	}
}
