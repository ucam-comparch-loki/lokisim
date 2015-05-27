/*
 * NetworkHierarchy.cpp
 *
 *  Created on: 1 Nov 2010
 *      Author: db434
 */

#include "NetworkHierarchy.h"
#include "Topologies/LocalNetwork.h"
#include "../Utility/Parameters.h"

local_net_t* NetworkHierarchy::getLocalNetwork(ComponentID component) const {
  unsigned int tileColumn = component.tile.x;
  unsigned int tileRow = component.tile.y;
  unsigned int tile = ((tileRow-1) * COMPUTE_TILE_COLUMNS) + (tileColumn-1);
  assert(tileColumn <= COMPUTE_TILE_COLUMNS);
  assert(tileRow <= COMPUTE_TILE_ROWS);
  assert(tile < localNetworks.size());
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

  int portIndex = tile * INPUT_PORTS_PER_TILE;
  for (unsigned int i=0; i<INPUT_PORTS_PER_TILE; i++, portIndex++)
    localNetwork->oData[i](oData[portIndex]);

  portIndex = tile * INPUT_CHANNELS_PER_TILE;
  for (unsigned int i=0; i<COMPONENTS_PER_TILE; i++) {
    unsigned int channels = (i < CORES_PER_TILE) ? CORE_INPUT_CHANNELS : 1;
    for (unsigned int j=0; j<channels; j++, portIndex++)
      localNetwork->iReady[i][j](iReady[portIndex]);
  }

  portIndex = tile * OUTPUT_PORTS_PER_TILE;
  for (unsigned int i=0; i<OUTPUT_PORTS_PER_TILE; i++, portIndex++)
    localNetwork->iData[i](iData[portIndex]);
}

NetworkHierarchy::NetworkHierarchy(sc_module_name name) :
    Component(name) {

  // Make ports.
  // There are data ports for all components, but credit ports only for cores.
  iData.init(TOTAL_OUTPUT_PORTS);
  oData.init(TOTAL_INPUT_PORTS);

  int readyPorts = (CORES_PER_TILE * CORE_INPUT_CHANNELS + MEMS_PER_TILE) * NUM_COMPUTE_TILES;
  iReady.init(readyPorts);

  // Make a local network for each tile. This includes all wiring up and
  // creation of a router.
  for (uint col = 1; col <= COMPUTE_TILE_COLUMNS; col++) {
    for (uint row = 1; row <= COMPUTE_TILE_ROWS; row++) {
      makeLocalNetwork(col, row);
    }
  }

}

NetworkHierarchy::~NetworkHierarchy() {
  for (uint i=0; i<localNetworks.size(); i++)
    delete localNetworks[i];
}
