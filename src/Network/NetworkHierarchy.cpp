/*
 * NetworkHierarchy.cpp
 *
 *  Created on: 1 Nov 2010
 *      Author: db434
 */

#include "NetworkHierarchy.h"
#include "FlowControl/FlowControlIn.h"
#include "FlowControl/FlowControlOut.h"
#include "Topologies/LocalNetwork.h"
#include "Topologies/Mesh.h"

local_net_t* NetworkHierarchy::getLocalNetwork(ComponentID component) const {
  unsigned int tile = component.getTile();
  assert(tile < localNetworks.size());
  return localNetworks[tile];
}

void NetworkHierarchy::setupFlowControl() {

  // All cores and memories are now responsible for their own flow control.

  // Attach flow control units to the off-chip component too.
  FlowControlIn*  fcin  = new FlowControlIn("fc_in", ComponentID(), ChannelID());
  flowControlIn.push_back(fcin);
  FlowControlOut* fcout = new FlowControlOut("fc_out", ComponentID(), ChannelID());
  flowControlOut.push_back(fcout);

  fcin->clock(clock);
  fcin->dataOut(dataToOffChip);            offChip.dataIn(dataToOffChip);
                                           offChip.readyOut(readyDataFromOffchip);
  fcin->dataIn(dataFromOffchip);
  fcin->creditsOut(creditsToOffchip);
  fcin->dataConsumed(dataConsumedOffchip); offChip.dataConsumed(dataConsumedOffchip);

  fcout->dataIn(dataFromOffChip);          offChip.dataOut(dataFromOffChip);
  fcout->flowControlOut(readyToOffChip);   offChip.readyIn(readyToOffChip);
  fcout->dataOut(dataToOffchip);
  fcout->readyIn(readyDataFromOffchip);
  fcout->creditsIn(creditsFromOffchip);
  fcout->readyOut(readyCreditFromOffchip);

}

void NetworkHierarchy::makeLocalNetwork(int tileID) {

  // Create a local network.
  local_net_t* localNetwork = new local_net_t(sc_gen_unique_name("tile_net"), ComponentID(tileID, 0));
  localNetworks.push_back(localNetwork);

  // Connect things up.
  localNetwork->clock(clock);
  localNetwork->fastClock(fastClock);
  localNetwork->slowClock(slowClock);

  int portIndex = tileID * INPUT_PORTS_PER_TILE;
  for (unsigned int i=0; i<INPUT_PORTS_PER_TILE; i++, portIndex++)
    localNetwork->dataOut[i](dataOut[portIndex]);

  portIndex = tileID * INPUT_CHANNELS_PER_TILE;
  for (unsigned int i=0; i<COMPONENTS_PER_TILE; i++) {
    unsigned int channels = (i < CORES_PER_TILE) ? CORE_INPUT_CHANNELS : 1;
    for (unsigned int j=0; j<channels; j++, portIndex++)
      localNetwork->readyDataIn[i][j](readyDataIn[portIndex]);
  }

  portIndex = tileID * OUTPUT_PORTS_PER_TILE;
  for (unsigned int i=0; i<OUTPUT_PORTS_PER_TILE; i++, portIndex++)
    localNetwork->dataIn[i](dataIn[portIndex]);

  // Memories don't have credit connections.
  portIndex = tileID * CORES_PER_TILE;
  for (unsigned int i=0; i<CORES_PER_TILE; i++, portIndex++)
    localNetwork->creditsIn[i](creditsIn[portIndex]);

  portIndex = tileID * CORES_PER_TILE * CORE_OUTPUT_PORTS;
  for (unsigned int i=0; i<CORES_PER_TILE*CORE_OUTPUT_PORTS; i++, portIndex++) {
    localNetwork->creditsOut[i](creditsOut[portIndex]);
    localNetwork->readyCreditsIn[i][0](readyCreditsIn[portIndex]);
  }

  // Simplify the network if there is only one tile: there is no longer any
  // need for a global network, so this local network can connect directly
  // to the OffChip component.
  if (NUM_TILES == 1) {
    localNetwork->externalInput()(dataToOffchip);
    localNetwork->externalOutput()(dataFromOffchip);
    localNetwork->externalDataReady()(readyDataFromOffchip);

    localNetwork->externalCreditIn()(creditsToOffchip);
    localNetwork->externalCreditOut()(creditsFromOffchip);
    localNetwork->externalCreditReady()(readyCreditFromOffchip);
  }
}

void NetworkHierarchy::makeGlobalNetwork() {

  // Make data network.
  globalDataNetwork = new global_net_t("global_data_net",
                                       0,
                                       NUM_TILE_ROWS,
                                       NUM_TILE_COLUMNS,
                                       false,
                                       Network::TILE);  // This network connects tiles
                               
  globalDataNetwork->clock(clock);

  for (uint i=0; i<localNetworks.size(); i++) {
    local_net_t* n = localNetworks[i];
    n->externalInput()(dataToLocalNet[i]);
    n->externalOutput()(dataFromLocalNet[i]);
    n->externalDataReady()(globalReadyForData[i]);
    globalDataNetwork->dataOut[i](dataToLocalNet[i]);
    globalDataNetwork->dataIn[i](dataFromLocalNet[i]);
    globalDataNetwork->readyOut[i](globalReadyForData[i]);
  }

  // Make credit network.
  // TODO: integrate both networks into one GlobalNetwork.
  globalCreditNetwork = new global_net_t("global_credit_net",
                                         0,
                                         NUM_TILE_ROWS,
                                         NUM_TILE_COLUMNS,
                                         true,
                                         Network::TILE);  // This network connects tiles

  globalCreditNetwork->clock(clock);

  for (uint i=0; i<localNetworks.size(); i++) {
    local_net_t* n = localNetworks[i];
    n->externalCreditIn()(creditsToLocalNet[i]);
    n->externalCreditOut()(creditsFromLocalNet[i]);
    n->externalCreditReady()(globalReadyForCredits[i]);
    globalCreditNetwork->dataOut[i](creditsToLocalNet[i]);
    globalCreditNetwork->dataIn[i](creditsFromLocalNet[i]);
    globalCreditNetwork->readyOut[i](globalReadyForCredits[i]);
  }

}

NetworkHierarchy::NetworkHierarchy(sc_module_name name) :
    Component(name),
    offChip("offchip") {

  // Make ports.
  // There are data ports for all components, but credit ports only for cores.
  dataIn.init(TOTAL_OUTPUT_PORTS);
  dataOut.init(TOTAL_INPUT_PORTS);

  int readyPorts = (CORES_PER_TILE * CORE_INPUT_CHANNELS + MEMS_PER_TILE) * NUM_TILES;
  readyDataIn.init(readyPorts);

  creditsIn.init(NUM_CORES);
  creditsOut.init(CORE_OUTPUT_PORTS * NUM_CORES);
  readyCreditsIn.init(CORE_OUTPUT_PORTS * NUM_CORES);

  // Make wires between local and global networks.
  dataFromLocalNet.init(NUM_TILES);
  dataToLocalNet.init(NUM_TILES);
  creditsFromLocalNet.init(NUM_TILES);
  creditsToLocalNet.init(NUM_TILES);
  localReadyForData.init(NUM_TILES);
  localReadyForCredits.init(NUM_TILES);
  globalReadyForData.init(NUM_TILES);
  globalReadyForCredits.init(NUM_TILES);

  // Make flow control (only necessary for off-chip component - integrate there
  // too?)
  setupFlowControl();

  // Make a local network for each tile. This includes all wiring up and
  // creation of a router.
  for (unsigned int tile=0; tile<NUM_TILES; tile++) {
    makeLocalNetwork(tile);
  }

  // Only need a global network if there is more than one tile to link together.
  if (NUM_TILES>1) makeGlobalNetwork();

}

NetworkHierarchy::~NetworkHierarchy() {
  for (uint i=0; i<flowControlIn.size(); i++) delete flowControlIn[i];
  for (uint i=0; i<flowControlOut.size(); i++) delete flowControlOut[i];

  // Delete networks
  for (uint i=0; i<localNetworks.size(); i++) delete localNetworks[i];

  // The global networks won't exist if there is only one tile, so we can't
  // delete them.
  if (NUM_TILES>1) {
    delete globalDataNetwork;
    delete globalCreditNetwork;
  }
}
