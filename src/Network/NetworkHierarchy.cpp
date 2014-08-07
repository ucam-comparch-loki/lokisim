/*
 * NetworkHierarchy.cpp
 *
 *  Created on: 1 Nov 2010
 *      Author: db434
 */

#include "NetworkHierarchy.h"
#include "Topologies/LocalNetwork.h"

local_net_t* NetworkHierarchy::getLocalNetwork(ComponentID component) const {
  unsigned int tile = component.getTile();
  assert(tile < localNetworks.size());
  return localNetworks[tile];
}

void NetworkHierarchy::makeLocalNetwork(int tileID) {

  // Create a local network.
  local_net_t* localNetwork = new local_net_t(sc_gen_unique_name("local"), ComponentID(tileID, 0));
  localNetworks.push_back(localNetwork);

  // Connect things up.
  localNetwork->clock(clock);
  localNetwork->fastClock(fastClock);
  localNetwork->slowClock(slowClock);

  int portIndex = tileID * INPUT_PORTS_PER_TILE;
  for (unsigned int i=0; i<INPUT_PORTS_PER_TILE; i++, portIndex++)
    localNetwork->oData[i](oData[portIndex]);

  portIndex = tileID * INPUT_CHANNELS_PER_TILE;
  for (unsigned int i=0; i<COMPONENTS_PER_TILE; i++) {
    unsigned int channels = (i < CORES_PER_TILE) ? CORE_INPUT_CHANNELS : 1;
    for (unsigned int j=0; j<channels; j++, portIndex++)
      localNetwork->iReady[i][j](iReady[portIndex]);
  }

  portIndex = tileID * OUTPUT_PORTS_PER_TILE;
  for (unsigned int i=0; i<OUTPUT_PORTS_PER_TILE; i++, portIndex++)
    localNetwork->iData[i](iData[portIndex]);
}

NetworkHierarchy::NetworkHierarchy(sc_module_name name) :
    Component(name) {

  // Make ports.
  // There are data ports for all components, but credit ports only for cores.
  iData.init(TOTAL_OUTPUT_PORTS);
  oData.init(TOTAL_INPUT_PORTS);

  int readyPorts = (CORES_PER_TILE * CORE_INPUT_CHANNELS + MEMS_PER_TILE) * NUM_TILES;
  iReady.init(readyPorts);

  // Make a local network for each tile. This includes all wiring up and
  // creation of a router.
  for (unsigned int tile=0; tile<NUM_TILES; tile++) {
    makeLocalNetwork(tile);
  }

}

NetworkHierarchy::~NetworkHierarchy() {
  for (uint i=0; i<localNetworks.size(); i++)
    delete localNetworks[i];
}
