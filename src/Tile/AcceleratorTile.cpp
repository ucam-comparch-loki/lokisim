/*
 * AcceleratorTile.cpp
 *
 *  Created on: 13 Aug 2018
 *      Author: db434
 */

#include "AcceleratorTile.h"
#include "Accelerator/Accelerator.h"

AcceleratorTile::AcceleratorTile(const sc_module_name& name,
                                 const ComponentID& id) :
    ComputeTile(name, id) {

  TileID tile = id.tile;

  // Create accelerators.
  for (uint acc = 0; acc < ACCELERATORS_PER_TILE; acc++) {
    ComponentID accID(tile, acc + CORES_PER_TILE + MEMS_PER_TILE);

    Accelerator* a = new Accelerator(sc_gen_unique_name("acc"), accID);
    accelerators.push_back(a);

    a->iClock(iClock);
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

