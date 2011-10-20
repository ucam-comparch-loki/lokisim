/*
 * NetworkHierarchy.cpp
 *
 *  Created on: 1 Nov 2010
 *      Author: db434
 */

#include "NetworkHierarchy.h"
#include "FlowControl/FlowControlIn.h"
#include "FlowControl/FlowControlOut.h"
#include "Topologies/Crossbar.h"
#include "Topologies/MulticastCrossbar.h"
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
  fcin->validDataIn(validDataFromOffchip);
  fcin->creditsOut(creditsToOffchip);
  fcin->validCreditOut(validCreditToOffchip);
  fcin->ackCreditOut(ackCreditToOffchip);

  fcout->dataIn(dataFromOffChip);          offChip.dataOut(dataFromOffChip);
  fcout->flowControlOut(readyToOffChip);   offChip.readyIn(readyToOffChip);
  fcout->dataOut(dataToOffchip);
  fcout->readyIn(readyDataFromOffchip);
  fcout->creditsIn(creditsFromOffchip);
  fcout->readyOut(ackCreditFromOffchip);

}

void NetworkHierarchy::makeLocalNetwork(int tileID) {

  // Create a local network.
  local_net_t* localNetwork = new local_net_t(sc_gen_unique_name("tile_net"), ComponentID(tileID, 0));
  localNetworks.push_back(localNetwork);

  // Connect things up.
  localNetwork->clock(clock);
  localNetwork->fastClock(fastClock);
  localNetwork->slowClock(slowClock);

  for(unsigned int i=0; i<INPUT_PORTS_PER_TILE; i++) {
    int outputIndex = (tileID * INPUT_PORTS_PER_TILE) + i;
    localNetwork->dataOut[i](dataOut[outputIndex]);
    localNetwork->validDataOut[i](validDataOut[outputIndex]);
  }

  for(unsigned int i=0; i<COMPONENTS_PER_TILE; i++) {
    int outputIndex = (tileID * COMPONENTS_PER_TILE) + i;
    localNetwork->readyIn[i](readyDataOut[outputIndex]);
  }

  for(unsigned int i=0; i<OUTPUT_PORTS_PER_TILE; i++) {
    int inputIndex = (tileID * OUTPUT_PORTS_PER_TILE) + i;
    localNetwork->dataIn[i](dataIn[inputIndex]);
    localNetwork->validDataIn[i](validDataIn[inputIndex]);
  }

  // Memories don't have credit connections.
  for(unsigned int i=0; i<CORES_PER_TILE; i++) {
    int outputIndex = (tileID * CORES_PER_TILE) + i;
    localNetwork->creditsIn[i](creditsIn[outputIndex]);
    localNetwork->validCreditIn[i](validCreditIn[outputIndex]);
    localNetwork->ackCreditIn[i](ackCreditIn[outputIndex]);
  }

  for(unsigned int i=0; i<CORES_PER_TILE*CORE_OUTPUT_PORTS; i++) {
    int inputIndex = (tileID * CORES_PER_TILE * CORE_OUTPUT_PORTS) + i;
    localNetwork->creditsOut[i](creditsOut[inputIndex]);
    localNetwork->validCreditOut[i](validCreditOut[inputIndex]);
    localNetwork->ackCreditOut[i](ackCreditOut[inputIndex]);
  }

  // Simplify the network if there is only one tile: there is no longer any
  // need for a global network, so this local network can connect directly
  // to the OffChip component.
  if(NUM_TILES == 1) {
    localNetwork->externalInput()(dataToOffchip);
    localNetwork->externalValidInput()(validDataToOffchip);
    localNetwork->externalOutput()(dataFromOffchip);
    localNetwork->externalValidOutput()(validDataToOffchip);
    localNetwork->externalReadyInput()(readyDataFromOffchip);

    localNetwork->externalCreditIn()(creditsToOffchip);
    localNetwork->externalValidCreditIn()(validCreditToOffchip);
    localNetwork->externalAckCreditOut()(ackCreditToOffchip);
    localNetwork->externalCreditOut()(creditsFromOffchip);
    localNetwork->externalValidCreditOut()(validCreditFromOffchip);
    localNetwork->externalAckCreditIn()(ackCreditFromOffchip);
  }
}

void NetworkHierarchy::makeGlobalNetwork() {

  // Make data network.
  globalDataNetwork = new global_net_t("global_data_net",
                                       0,
                                       NUM_TILE_ROWS,
                                       NUM_TILE_COLUMNS,
                                       Network::TILE,   // This network connects tiles
                                       Dimension(NUM_TILES, NUM_TILES));
                               
  globalDataNetwork->clock(clock);

  for(uint i=0; i<localNetworks.size(); i++) {
    local_net_t* n = localNetworks[i];
    n->externalInput()(dataToLocalNet[i]);
    n->externalOutput()(dataFromLocalNet[i]);
    n->externalValidInput()(validDataToLocal[i]);
    n->externalValidOutput()(validDataFromLocal[i]);
    n->externalReadyInput()(globalReadyForData[i]);
    globalDataNetwork->dataOut[i](dataToLocalNet[i]);
    globalDataNetwork->dataIn[i](dataFromLocalNet[i]);
    globalDataNetwork->validDataOut[i](validDataToLocal[i]);
    globalDataNetwork->validDataIn[i](validDataFromLocal[i]);
    globalDataNetwork->ackDataIn[i](globalReadyForData[i]);
    globalDataNetwork->ackDataOut[i](localReadyForData[i]);
  }

  // Make credit network.
  // TODO: integrate both networks into one GlobalNetwork.
  globalCreditNetwork = new global_net_t("global_credit_net",
                                         0,
                                         NUM_TILE_ROWS,
                                         NUM_TILE_COLUMNS,
                                         Network::TILE,   // This network connects tiles
                                         Dimension(NUM_TILES, NUM_TILES));

  globalCreditNetwork->clock(clock);

  for(uint i=0; i<localNetworks.size(); i++) {
    local_net_t* n = localNetworks[i];
    n->externalCreditIn()(creditsToLocalNet[i]);
    n->externalCreditOut()(creditsFromLocalNet[i]);
    n->externalValidCreditIn()(validCreditToLocal[i]);
    n->externalValidCreditOut()(validCreditFromLocal[i]);
    n->externalAckCreditIn()(globalReadyForCredits[i]);
    n->externalAckCreditOut()(localReadyForCredits[i]);
    globalCreditNetwork->dataOut[i](creditsToLocalNet[i]);
    globalCreditNetwork->dataIn[i](creditsFromLocalNet[i]);
    globalCreditNetwork->validDataOut[i](validCreditToLocal[i]);
    globalCreditNetwork->validDataIn[i](validCreditFromLocal[i]);
    globalCreditNetwork->ackDataIn[i](globalReadyForCredits[i]);
    globalCreditNetwork->ackDataOut[i](localReadyForCredits[i]);
  }

}

NetworkHierarchy::NetworkHierarchy(sc_module_name name) :
    Component(name),
    offChip("offchip") {

  // Make ports.
  // There are data ports for all components, but credit ports only for cores.
  dataIn                = new DataInput[TOTAL_OUTPUT_PORTS];
  dataOut               = new DataOutput[TOTAL_INPUT_PORTS];
  validDataIn           = new ReadyInput[TOTAL_OUTPUT_PORTS];
  validDataOut          = new ReadyOutput[TOTAL_INPUT_PORTS];
  readyDataOut          = new ReadyInput[NUM_COMPONENTS];
  creditsIn             = new CreditInput[NUM_CORES];
  creditsOut            = new CreditOutput[CORE_OUTPUT_PORTS * NUM_CORES];
  validCreditIn         = new ReadyInput[NUM_CORES];
  validCreditOut        = new ReadyOutput[CORE_OUTPUT_PORTS * NUM_CORES];
  ackCreditIn           = new ReadyOutput[NUM_CORES];
  ackCreditOut          = new ReadyInput[CORE_OUTPUT_PORTS * NUM_CORES];

  // Make wires between local and global networks.
  dataFromLocalNet      = new DataSignal[NUM_TILES];
  dataToLocalNet        = new DataSignal[NUM_TILES];
  creditsFromLocalNet   = new CreditSignal[NUM_TILES];
  creditsToLocalNet     = new CreditSignal[NUM_TILES];
  validDataToLocal      = new ReadySignal[NUM_TILES];
  validDataFromLocal    = new ReadySignal[NUM_TILES];
  validCreditToLocal    = new ReadySignal[NUM_TILES];
  validCreditFromLocal  = new ReadySignal[NUM_TILES];
  localReadyForData     = new ReadySignal[NUM_TILES];
  localReadyForCredits  = new ReadySignal[NUM_TILES];
  globalReadyForData    = new ReadySignal[NUM_TILES];
  globalReadyForCredits = new ReadySignal[NUM_TILES];

  // Make flow control (only necessary for off-chip component - integrate there
  // too?)
  setupFlowControl();

  // Make a local network for each tile. This includes all wiring up and
  // creation of a router.
  for(unsigned int tile=0; tile<NUM_TILES; tile++) {
    makeLocalNetwork(tile);
  }

  // Only need a global network if there is more than one tile to link together.
  if(NUM_TILES>1) makeGlobalNetwork();

}

NetworkHierarchy::~NetworkHierarchy() {
  delete[] dataIn;                    delete[] dataOut;
  delete[] creditsIn;                 delete[] creditsOut;
  delete[] validDataIn;               delete[] validDataOut;
  delete[] validCreditIn;             delete[] validCreditOut;
  delete[] readyDataOut;
  delete[] ackCreditIn;               delete[] ackCreditOut;

  delete[] dataToLocalNet;            delete[] dataFromLocalNet;
  delete[] creditsToLocalNet;         delete[] creditsFromLocalNet;
  delete[] validDataToLocal;          delete[] validDataFromLocal;
  delete[] validCreditToLocal;        delete[] validCreditFromLocal;
  delete[] localReadyForData;         delete[] globalReadyForData;
  delete[] localReadyForCredits;      delete[] globalReadyForCredits;

  for(uint i=0; i<flowControlIn.size(); i++) delete flowControlIn[i];
  for(uint i=0; i<flowControlOut.size(); i++) delete flowControlOut[i];

  // Delete networks
  for(uint i=0; i<localNetworks.size(); i++) delete localNetworks[i];

  // The global networks won't exist if there is only one tile, so we can't
  // delete them.
  if(NUM_TILES>1) {
    delete globalDataNetwork;
    delete globalCreditNetwork;
  }
}
