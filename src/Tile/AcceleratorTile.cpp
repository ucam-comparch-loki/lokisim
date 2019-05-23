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
                                 const TileID& id,
                                 const tile_parameters_t& params) :
    ComputeTile(name, id, params) {

  // Create accelerators.
  for (uint acc = 0; acc < params.numAccelerators; acc++) {
    ComponentID accID(id, acc + params.numCores + params.numMemories);

    Accelerator* a = new Accelerator(sc_gen_unique_name("acc"), accID,
                                     params.accelerator);
    accelerators.push_back(a);

    a->clock(clock);

    // Connect to the multicast network. Some slightly awkward indexing because
    // all of the cores have already been connected.
    coreToCore.inputs[params.numCores + acc](a->oMulticast);
    coreToCore.outputs[params.numCores*params.core.numInputChannels + acc](a->iMulticast[0]);
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
  if (flit.channelID().component.tile != id)
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

