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
                                 const ComponentID& id) :
    ComputeTile(name, id) {

  TileID tile = id.tile;

  // Create accelerators.
  for (uint acc = 0; acc < ACCELERATORS_PER_TILE; acc++) {
    ComponentID accID(tile, acc + CORES_PER_TILE + MEMS_PER_TILE);

    // TODO Get this configuration from somewhere useful.
    Configuration cfg(4, 4, true, false, true, false, LoopOrders::naive);

    Accelerator* a = new Accelerator(sc_gen_unique_name("acc"), accID, cfg);
    accelerators.push_back(a);

    a->iClock(iClock);

    // Connect to the multicast network. Some slightly awkward indexing because
    // all of the cores have already been connected.
    a->iMulticast(multicastToCores[CORES_PER_TILE]); // vector
    a->oMulticast(multicastFromCores[CORES_PER_TILE]);
    a->oReadyData(readyDataFromCores[CORES_PER_TILE]); // vector
  }

}

AcceleratorTile::~AcceleratorTile() {
  for (uint i=0; i<accelerators.size(); i++)
    delete accelerators[i];
}

void AcceleratorTile::networkSendDataInternal(const NetworkData& flit) {
  if (flit.channelID().component.tile != id.tile)
    chip()->networkSendDataInternal(flit);
  else if (flit.channelID().isCore()) {
    cores[flit.channelID().component.position]->deliverDataInternal(flit);
  }
  else if (flit.channelID().isMemory()) {
    loki_assert_with_message(false, "should never touch memory banks when"
                                    " using magic memory", 0);
  }
  else { // Accelerator
    // Some slightly awkward network addressing at the moment.
    // Components are assigned IDs in this order: cores, memories, accelerators.
    // Accelerators also have multiple network access points, so multiple
    // addresses map to the same accelerator.
    uint index = flit.channelID().component.position - CORES_PER_TILE - MEMS_PER_TILE;
    uint component = index % 3;

    accelerators[component]->deliverDataInternal(flit);
  }
}

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

    Accelerator* a = new Accelerator(sc_gen_unique_name("acc"), accID,
                                     params.accelerator,
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
                                     params.numMemories, params.accelerator);
    accelerators.push_back(a);

    a->clock(clock);

    // Connect to the existing networks. Some slightly awkward indexing because
    // all of the cores have already been connected.

    // Multicast.
    coreToCore.inputs[params.numCores + acc](a->oMulticast);
    coreToCore.outputs[params.numCores*params.core.numInputChannels + acc](a->iMulticast[0]);

    // To/from memory.
    // Ports connected in ComputeTile.
    uint c2mPorts = params.numCores;
    uint m2cPorts = params.numCores * params.core.numInputChannels;

    for (uint i=0; i<accelerators.size(); i++) {
      // Core to memory.
      for (uint j=0; j<accelerators[i].oMemory.size(); j++) {
        for (uint k=0; k<accelerators[i].oMemory[j].size(); k++) {
          coreToMemory.inputs[c2mPorts](accelerators[i].oMemory[j][k]);
          c2mPorts++;
        }
      }

      // Memory to cores (data).
      for (uint j=0; j<accelerators[i].iMemory.size(); j++) {
        for (uint k=0; k<accelerators[i].iMemory[j].size(); k++) {
          dataReturn.outputs[m2cPorts](accelerators[i].iMemory[j][k]);
          m2cPorts++;
        }
      }
    }

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

