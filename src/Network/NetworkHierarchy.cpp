/*
 * NetworkHierarchy.cpp
 *
 *  Created on: 1 Nov 2010
 *      Author: db434
 */

#include "NetworkHierarchy.h"
#include "Topologies/LocalNetwork.h"
#include "../Utility/Assert.h"
#include "../Utility/Parameters.h"

local_net_t* NetworkHierarchy::getLocalNetwork(ComponentID component) const {
  unsigned int tile = component.tile.computeTileIndex();
  loki_assert(component.tile.x <= COMPUTE_TILE_COLUMNS);
  loki_assert(component.tile.y <= COMPUTE_TILE_ROWS);
  loki_assert(tile < localNetworks.size());
  return localNetworks[tile];
}

void NetworkHierarchy::makeLocalNetwork(uint tileColumn, uint tileRow) {
  // Create a local network.
  ComponentID tileID(tileColumn, tileRow, 0);
  local_net_t* localNetwork = new local_net_t(sc_gen_unique_name("local"), tileID);
  localNetworks.push_back(localNetwork);

  // Connect things up.
  localNetwork->clock(clock);
  localNetwork->fastClock(fastClock);
  localNetwork->slowClock(slowClock);

  uint tile = tileID.tile.computeTileIndex();

  localNetwork->iData(iData[tile]);
  localNetwork->oData(oData[tile]);
  localNetwork->iReady(iReady[tile]);
}

NetworkHierarchy::NetworkHierarchy(sc_module_name name) :
    LokiComponent(name) {

  // Make ports. Note that the network's inputs are connected to the cores'
  // outputs, and vice versa.
  iData.init(NUM_COMPUTE_TILES);
  oData.init(NUM_COMPUTE_TILES);
  iReady.init(NUM_COMPUTE_TILES);

  for (uint tile=0; tile<iReady.length(); tile++) {
    iData[tile].init(COMPONENTS_PER_TILE);
    oData[tile].init(COMPONENTS_PER_TILE);
    iReady[tile].init(COMPONENTS_PER_TILE);

    for (uint component=0; component<CORES_PER_TILE; component++) {
      iData[tile][component].init(CORE_OUTPUT_PORTS);
      oData[tile][component].init(CORE_INPUT_PORTS);
      iReady[tile][component].init(CORE_INPUT_CHANNELS);
    }
    for (uint component=CORES_PER_TILE; component<COMPONENTS_PER_TILE; component++) {
      iData[tile][component].init(1);
      oData[tile][component].init(1);
      iReady[tile][component].init(1);
    }
  }

  // Make a local network for each tile. This includes all wiring up and
  // creation of a router.
  for (uint row = 1; row <= COMPUTE_TILE_ROWS; row++) {
    for (uint col = 1; col <= COMPUTE_TILE_COLUMNS; col++) {
      makeLocalNetwork(col, row);
    }
  }

}

NetworkHierarchy::~NetworkHierarchy() {
  for (uint i=0; i<localNetworks.size(); i++)
    delete localNetworks[i];
}
