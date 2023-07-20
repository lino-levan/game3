#include "Directions.h"
#include "pipes/ItemNetwork.h"
#include "pipes/PipeLoader.h"
#include "pipes/PipeNetwork.h"
#include "realm/Realm.h"
#include "tileentity/Pipe.h"

#include <atomic>

namespace Game3 {
	void PipeLoader::load(Realm &realm, ChunkPosition chunk_position) {
		{
			auto shared_lock = busyChunks.sharedLock();
			if (busyChunks.contains(chunk_position))
				return;
		}

		{
			auto unique_lock = busyChunks.uniqueLock();
			if (busyChunks.contains(chunk_position))
				return;
			busyChunks.insert(chunk_position);
		}

		if (auto by_chunk = realm.getTileEntities(chunk_position)) {
			auto lock = by_chunk->sharedLock();
			for (const TileEntityPtr &tile_entity: *by_chunk)
				if (auto pipe = tile_entity->cast<Pipe>())
					for (const PipeType pipe_type: {PipeType::Item, PipeType::Fluid, PipeType::Energy})
						if (!pipe->loaded[pipe_type])
							floodFill(pipe_type, pipe);
		}

		{
			auto unique_lock = busyChunks.uniqueLock();
			busyChunks.erase(chunk_position);
		}
	}

	void PipeLoader::floodFill(PipeType pipe_type, const std::shared_ptr<Pipe> &start) {
		// The initial pipe needs to have not been loaded yet, and it can't already have a network.
		assert(!start->loaded[pipe_type]);
		// If this assertion ever fails, something is horribly wrong.
		assert(!start->getNetwork(pipe_type));

		std::shared_ptr<PipeNetwork> network;
		auto realm = start->getRealm();

		switch (pipe_type) {
			case PipeType::Item:
				network = std::make_shared<ItemNetwork>(++lastID, realm);
				break;
			case PipeType::Fluid:
			case PipeType::Energy:
				return;
			default:
				throw std::invalid_argument("Invalid PipeType");
		}

		network->add(start);

		std::vector<std::shared_ptr<Pipe>> queue{start};

		while (!queue.empty()) {
			const std::shared_ptr<Pipe> pipe = std::move(queue.back());
			queue.pop_back();

			Directions &directions = pipe->getDirections()[pipe_type];

			if (auto other_network = pipe->getNetwork(pipe_type)) {
				if (network != other_network)
					network->absorb(other_network);
			} else
				network->add(pipe);

			directions.iterate([&](Direction direction) {
				const Position neighbor_position = pipe->getPosition() + direction;
				if (TileEntityPtr base_neighbor = realm->tileEntityAt(neighbor_position)) {
					if (auto neighbor = base_neighbor->cast<Pipe>()) {
						// Check whether the connection is matched with a connection on the other pipe.
						if (!neighbor->loaded[pipe_type] && neighbor->getDirections()[pipe_type][flipDirection(direction)])
							queue.push_back(neighbor);
					}
				}
			});
		}
	}
}
