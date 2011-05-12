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

void NetworkHierarchy::setupFlowControl() {

  // All cores and memories are now responsible for their own flow control.

  // Attach flow control units to the off-chip component too.
  FlowControlIn*  fcin  = new FlowControlIn("fc_in", ComponentID()/*TOTAL_INPUT_PORTS*/, ChannelID());
  flowControlIn.push_back(fcin);
  FlowControlOut* fcout = new FlowControlOut("fc_out", ComponentID()/*TOTAL_OUTPUT_PORTS*/, ChannelID());
  flowControlOut.push_back(fcout);

  fcin->dataOut(dataToOffChip);            offChip.dataIn(dataToOffChip);
  fcin->creditsIn(creditsFromOffChip);     offChip.creditsOut(creditsFromOffChip);
  fcin->dataIn(dataToComponents[0]);
  fcin->validDataIn(validDataToComps[0]);
  fcin->ackDataIn(ackDataToComps[0]);
  fcin->creditsOut(creditsFromComponents[0]);
  fcin->validCreditOut(validCreditFromComps[0]);
  fcin->ackCreditOut(ackCreditFromComps[0]);

  fcout->dataIn(dataFromOffChip);          offChip.dataOut(dataFromOffChip);
  fcout->flowControlOut(readyToOffChip);   offChip.readyIn(readyToOffChip);
  fcout->dataOut(dataFromComponents[0]);
  fcout->readyIn(ackDataFromComps[0]);
  fcout->creditsIn(creditsToComponents[0]);
  fcout->readyOut(ackCreditToComps[0]);

}

void NetworkHierarchy::makeDataNetwork() {
  // Make a local network for each tile. This includes all wiring up and
  // creation of a router.
  for(uint tile=0; tile<NUM_TILES; tile++) {
    makeLocalDataNetwork(tile);
  }

  // Only need a global network if there is more than one tile to link together.
  if(NUM_TILES>1) makeGlobalDataNetwork();
}

void NetworkHierarchy::makeLocalDataNetwork(int tileID) {

  // Create a local network.
  ChannelID lowestID(tileID, 0, 0);	//TODO: Global IDs?
  Network* localNetwork = new Crossbar("data_net",
                                       tileID,
                                       OUTPUT_PORTS_PER_TILE,
                                       INPUT_PORTS_PER_TILE,
                                       1,//CORE_INPUT_PORTS,
                                       CORE_INPUT_CHANNELS/CORE_INPUT_PORTS,
                                       lowestID,
                                       Dimension(1.0, 0.2),
                                       true);
  localDataNetworks.push_back(localNetwork);

  // Connect things up.
  for(uint i=0; i<INPUT_PORTS_PER_TILE; i++) {
    int outputIndex = (tileID * INPUT_PORTS_PER_TILE) + i;
    localNetwork->dataOut[i](dataOut[outputIndex]);
    localNetwork->validDataOut[i](validDataOut[outputIndex]);
    localNetwork->ackDataOut[i](ackDataOut[outputIndex]);
  }
  for(uint i=0; i<OUTPUT_PORTS_PER_TILE; i++) {
    int inputIndex = (tileID * OUTPUT_PORTS_PER_TILE) + i;
    localNetwork->dataIn[i](dataIn[inputIndex]);
    localNetwork->validDataIn[i](validDataIn[inputIndex]);
    localNetwork->ackDataIn[i](ackDataIn[inputIndex]);
  }

  localNetwork->clock(clock);

  // Simplify the network if there is only one tile: there is no longer any
  // need for a global network, so this local network can connect directly
  // to the OffChip component.
  if(NUM_TILES == 1) {
    localNetwork->externalInput()(dataFromComponents[0]);
    localNetwork->externalValidInput()(validDataFromComps[0]);
    localNetwork->externalReadyOutput()(ackDataFromComps[0]);
    localNetwork->externalOutput()(dataToComponents[0]);
    localNetwork->externalValidOutput()(validDataFromComps[0]);
    localNetwork->externalReadyInput()(ackDataToComps[0]);
  }

}

void NetworkHierarchy::makeGlobalDataNetwork() {

  globalDataNetwork = new Crossbar("global_data_net",
                                   0,
                                   NUM_TILES,
                                   NUM_TILES,
                                   1,
                                   INPUT_CHANNELS_PER_TILE,
                                   0,
                                   Dimension(NUM_TILES, NUM_TILES),
                                   true);

  globalDataNetwork->clock(clock);

  // Connect the global network to the off-chip component.
  globalDataNetwork->externalOutput()(dataFromComponents[0]);
  globalDataNetwork->externalInput()(dataToComponents[0]);
  globalDataNetwork->externalValidOutput()(validDataFromComps[0]);
  globalDataNetwork->externalValidInput()(validDataToComps[0]);
  globalDataNetwork->externalReadyOutput()(ackDataToComps[0]);
  globalDataNetwork->externalReadyInput()(ackDataFromComps[0]);

  for(uint i=0; i<localDataNetworks.size(); i++) {
    Network* n = localDataNetworks[i];
    n->externalInput()(dataToLocalNet[i]);
    n->externalOutput()(dataFromLocalNet[i]);
    n->externalValidInput()(validDataToLocal[i]);
    n->externalValidOutput()(validDataFromLocal[i]);
    n->externalReadyInput()(globalReadyForData[i]);
    n->externalReadyOutput()(localReadyForData[i]);
    globalDataNetwork->dataOut[i](dataToLocalNet[i]);
    globalDataNetwork->dataIn[i](dataFromLocalNet[i]);
    globalDataNetwork->validDataOut[i](validDataToLocal[i]);
    globalDataNetwork->validDataIn[i](validDataFromLocal[i]);
    globalDataNetwork->ackDataIn[i](globalReadyForData[i]);
    globalDataNetwork->ackDataOut[i](localReadyForData[i]);
  }

}

void NetworkHierarchy::makeCreditNetwork() {
  // Make a local network for each tile. This includes all wiring up and
  // creation of a router.
  for(uint tile=0; tile<NUM_TILES; tile++) {
    makeLocalCreditNetwork(tile);
  }

  // Only need a global network if there is more than one tile to link together.
  if(NUM_TILES>1) makeGlobalCreditNetwork();
}

void NetworkHierarchy::makeLocalCreditNetwork(int tileID) {

  // Create a local network.
  ChannelID lowestID(tileID, 0, 0);	//TODO: Global IDs?
  Network* localNetwork = new Crossbar("credit_net",
                                       tileID,
                                       INPUT_PORTS_PER_TILE,
                                       OUTPUT_PORTS_PER_TILE,
                                       CORE_OUTPUT_PORTS,
                                       CORE_OUTPUT_CHANNELS,
                                       lowestID,
                                       Dimension(1.0, 0.2),
                                       true);
  localCreditNetworks.push_back(localNetwork);

  // Connect things up.
  for(uint i=0; i<OUTPUT_PORTS_PER_TILE; i++) {
    int outputIndex = (tileID * OUTPUT_PORTS_PER_TILE) + i;
    localNetwork->dataOut[i](creditsOut[outputIndex]);
    localNetwork->validDataOut[i](validCreditOut[outputIndex]);
    localNetwork->ackDataOut[i](ackCreditOut[outputIndex]);
  }
  for(uint i=0; i<INPUT_PORTS_PER_TILE; i++) {
    int inputIndex = (tileID * INPUT_PORTS_PER_TILE) + i;
    localNetwork->dataIn[i](creditsIn[inputIndex]);
    localNetwork->validDataIn[i](validCreditIn[inputIndex]);
    localNetwork->ackDataIn[i](ackCreditIn[inputIndex]);
  }

  localNetwork->clock(clock);

  // Simplify the network if there is only one tile: there is no longer any
  // need for a global network, and this local network can connect directly
  // to the OffChip component.
  if(NUM_TILES == 1) {
    localNetwork->externalInput()(creditsFromComponents[0]);
    localNetwork->externalValidInput()(validCreditFromComps[0]);
    localNetwork->externalReadyOutput()(ackCreditFromComps[0]);
    localNetwork->externalOutput()(creditsToComponents[0]);
    localNetwork->externalValidOutput()(validCreditToComps[0]);
    localNetwork->externalReadyInput()(ackCreditToComps[0]);
  }

}

void NetworkHierarchy::makeGlobalCreditNetwork() {

  globalDataNetwork = new Crossbar("global_credit_net",
                                   0,
                                   NUM_TILES,
                                   NUM_TILES,
                                   1,
                                   OUTPUT_CHANNELS_PER_TILE,
                                   0,
                                   Dimension(NUM_TILES, NUM_TILES),
                                   true);

  globalCreditNetwork->clock(clock);

  // Connect the global network to the off-chip component.
  globalCreditNetwork->externalInput()(creditsFromComponents[0]);
  globalCreditNetwork->externalOutput()(creditsToComponents[0]);
  globalCreditNetwork->externalValidInput()(validCreditFromComps[0]);
  globalCreditNetwork->externalValidOutput()(validCreditToComps[0]);
  globalCreditNetwork->externalReadyOutput()(ackCreditToComps[0]);
  globalCreditNetwork->externalReadyInput()(ackCreditFromComps[0]);

  for(uint i=0; i<localCreditNetworks.size(); i++) {
    Network* n = localCreditNetworks[i];
    n->externalInput()(creditsToLocalNet[i]);
    n->externalOutput()(creditsFromLocalNet[i]);
    n->externalValidInput()(validCreditToLocal[i]);
    n->externalValidOutput()(validCreditFromLocal[i]);
    n->externalReadyInput()(globalReadyForCredits[i]);
    n->externalReadyOutput()(localReadyForCredits[i]);
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
  dataIn                = new DataInput[TOTAL_OUTPUT_PORTS];
  dataOut               = new DataOutput[TOTAL_INPUT_PORTS];
  creditsIn             = new CreditInput[TOTAL_INPUT_PORTS];
  creditsOut            = new CreditOutput[TOTAL_OUTPUT_PORTS];
  validDataIn           = new ReadyInput[TOTAL_OUTPUT_PORTS];
  validDataOut          = new ReadyOutput[TOTAL_INPUT_PORTS];
  validCreditIn         = new ReadyInput[TOTAL_INPUT_PORTS];
  validCreditOut        = new ReadyOutput[TOTAL_OUTPUT_PORTS];
  ackDataIn             = new ReadyOutput[TOTAL_OUTPUT_PORTS];
  ackDataOut            = new ReadyInput[TOTAL_INPUT_PORTS];
  ackCreditIn           = new ReadyOutput[TOTAL_INPUT_PORTS];
  ackCreditOut          = new ReadyInput[TOTAL_OUTPUT_PORTS];

  // Make wires to the off-chip component.
  dataFromComponents    = new DataSignal[1];
  dataToComponents      = new DataSignal[1];
  creditsFromComponents = new CreditSignal[1];
  creditsToComponents   = new CreditSignal[1];
  validDataToComps      = new ReadySignal[1];
  validCreditToComps    = new ReadySignal[1];
  validDataFromComps    = new ReadySignal[1];
  validCreditFromComps  = new ReadySignal[1];
  ackDataToComps        = new ReadySignal[1];
  ackCreditToComps      = new ReadySignal[1];
  ackDataFromComps      = new ReadySignal[1];
  ackCreditFromComps    = new ReadySignal[1];

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

  // Make networks.
  setupFlowControl();
  makeDataNetwork();
  makeCreditNetwork();

}

NetworkHierarchy::~NetworkHierarchy() {
  delete[] dataIn;                    delete[] dataOut;
  delete[] creditsIn;                 delete[] creditsOut;
  delete[] validDataIn;               delete[] validDataOut;
  delete[] validCreditIn;             delete[] validCreditOut;
  delete[] ackDataIn;                 delete[] ackDataOut;
  delete[] ackCreditIn;               delete[] ackCreditOut;

  delete[] dataFromComponents;        delete[] dataToComponents;
  delete[] creditsFromComponents;     delete[] creditsToComponents;
  delete[] validDataToComps;          delete[] validDataFromComps;
  delete[] validCreditToComps;        delete[] validCreditFromComps;
  delete[] ackDataToComps;            delete[] ackDataFromComps;
  delete[] ackCreditToComps;          delete[] ackCreditFromComps;

  delete[] dataToLocalNet;            delete[] dataFromLocalNet;
  delete[] creditsToLocalNet;         delete[] creditsFromLocalNet;
  delete[] validDataToLocal;          delete[] validDataFromLocal;
  delete[] validCreditToLocal;        delete[] validCreditFromLocal;
  delete[] localReadyForData;         delete[] globalReadyForData;
  delete[] localReadyForCredits;      delete[] globalReadyForCredits;

  for(uint i=0; i<flowControlIn.size(); i++) delete flowControlIn[i];
  for(uint i=0; i<flowControlOut.size(); i++) delete flowControlOut[i];

  // Delete networks
  for(uint i=0; i<localDataNetworks.size(); i++) delete localDataNetworks[i];
  for(uint i=0; i<localCreditNetworks.size(); i++) delete localCreditNetworks[i];

  // The global networks won't exist if there is only one tile, so we can't
  // delete them.
  if(NUM_TILES>1) {
    delete globalDataNetwork;
    delete globalCreditNetwork;
  }
}
