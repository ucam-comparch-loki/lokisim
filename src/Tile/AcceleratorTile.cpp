/*
 * AcceleratorTile.cpp
 *
 *  Created on: 13 Aug 2018
 *      Author: db434
 */

#include "AcceleratorTile.h"
#include "Accelerator/Accelerator.h"
#include "../Chip.h"

AcceleratorTile::AcceleratorTile(const sc_module_name& name,
                                 const ComponentID& id,
                                 const tile_parameters_t& params) :
    ComputeTile(name, id, params) {

  TileID tile = id.tile;

  // Create accelerators.
  for (uint acc = 0; acc < params.numAccelerators; acc++) {
    ComponentID accID(tile, acc + params.numCores + params.numMemories);

    // TODO Get this configuration from somewhere useful.
    Configuration cfg(2, 2, false, false, false, false, LoopOrders::naive);

    Accelerator* a = new Accelerator(sc_gen_unique_name("acc"), accID, cfg,
                                     params.mcastNetOutputs());
    accelerators.push_back(a);

    a->iClock(iClock);

    // Connect to the multicast network. Some slightly awkward indexing because
    // all of the cores have already been connected.
    a->iMulticast(multicastToCores[params.numCores + acc]); // vector
    a->oMulticast(multicastFromCores[params.numCores + acc]);
    a->oReadyData(readyDataFromCores[params.numCores + acc]); // vector
  }

}

uint AcceleratorTile::numComponents() const {
  return numCores() + numMemories() + numAccelerators();
}

uint AcceleratorTile::numAccelerators() const {
  return accelerators.size();
}

bool AcceleratorTile::isCore(ComponentID id) const {
  return id.position < numCores();
}

bool AcceleratorTile::isMemory(ComponentID id) const {
  return (id.position >= numCores()) &&
         (id.position < numCores() + numMemories());
}

bool AcceleratorTile::isAccelerator(ComponentID id) const {
  return id.position >= numCores() + numMemories();
}

uint AcceleratorTile::componentIndex(ComponentID id) const {
  return id.position;
}

uint AcceleratorTile::coreIndex(ComponentID id) const {
  return id.position;
}

uint AcceleratorTile::memoryIndex(ComponentID id) const {
  return id.position - numCores();
}

uint AcceleratorTile::acceleratorIndex(ComponentID id) const {
  // Some slightly awkward network addressing at the moment.
  // Components are assigned IDs in this order: cores, memories, accelerators.
  // Accelerators also have multiple network access points, so multiple
  // addresses map to the same accelerator.
  return (id.position - numCores() - numMemories()) / 3;
}

void AcceleratorTile::networkSendDataInternal(const NetworkData& flit) {
  if (flit.channelID().component.tile != id.tile)
    chip().networkSendDataInternal(flit);
  else if (isCore(flit.channelID().component)) {
    cores[coreIndex(flit.channelID().component)].deliverDataInternal(flit);
  }
  else if (isMemory(flit.channelID().component)) {
    loki_assert_with_message(false, "should never touch memory banks when"
                                    " using magic memory", 0);
  }
  else { // Accelerator
    uint component = acceleratorIndex(flit.channelID().component);
    accelerators[component].deliverDataInternal(flit);
  }
}

