/*
 * NetworkHierarchy.cpp
 *
 *  Created on: 1 Nov 2010
 *      Author: db434
 */

#include "NetworkHierarchy.h"
#include "Network.h"
#include "Arbiters/Arbiter.h"
#include "FlowControl/FlowControlIn.h"
#include "FlowControl/FlowControlOut.h"
#include "Topologies/Crossbar.h"
#include "Topologies/NormalCrossbar.h"

void NetworkHierarchy::setupFlowControl() {

  // All cores and memories are now responsible for their own flow control.

  // Attach flow control units to the off-chip component too.
  FlowControlIn*  fcin  = new FlowControlIn("fc_in", TOTAL_INPUT_PORTS);
  flowControlIn.push_back(fcin);
  FlowControlOut* fcout = new FlowControlOut("fc_out", TOTAL_OUTPUT_PORTS);
  flowControlOut.push_back(fcout);

  fcin->dataOut(dataToOffChip);            offChip.dataIn(dataToOffChip);
  fcin->creditsIn(creditsFromOffChip);     offChip.creditsOut(creditsFromOffChip);
  fcin->dataIn(dataToComponents[/*TOTAL_INPUT_PORTS*/0]);
  fcin->canReceiveData(compReadyForData[/*TOTAL_INPUT_PORTS*/0]);
  fcin->creditsOut(creditsFromComponents[/*TOTAL_INPUT_PORTS*/0]);
  fcin->canSendCredits(readyForCredits[/*TOTAL_INPUT_PORTS*/0]);

  fcout->dataIn(dataFromOffChip);          offChip.dataOut(dataFromOffChip);
  fcout->flowControlOut(readyToOffChip);   offChip.readyIn(readyToOffChip);
  fcout->dataOut(dataFromComponents[/*TOTAL_OUTPUT_PORTS*/0]);
  fcout->readyIn(readyForData[/*TOTAL_OUTPUT_PORTS*/0]);
  fcout->creditsIn(creditsToComponents[/*TOTAL_OUTPUT_PORTS*/0]);
  fcout->readyOut(compReadyForCredits[/*TOTAL_OUTPUT_PORTS*/0]);

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
  ChannelID lowestID = tileID * INPUT_CHANNELS_PER_TILE;
//  ChannelID highestID = lowestID + INPUT_CHANNELS_PER_TILE - 1;
//  Arbiter* arbiter = Arbiter::localDataArbiter(OUTPUT_PORTS_PER_TILE, INPUT_PORTS_PER_TILE);
//  Network* localNetwork = new Crossbar("local_data_net",
//                                       tileID,
//                                       lowestID,
//                                       highestID,
//                                       OUTPUT_PORTS_PER_TILE,
//                                       INPUT_PORTS_PER_TILE,
//                                       RoutingComponent::LOC_DATA,
//                                       arbiter);
  NormalCrossbar* localNetwork = new NormalCrossbar("data_net",
                                                    tileID,
                                                    OUTPUT_PORTS_PER_TILE+1,
                                                    INPUT_PORTS_PER_TILE+1,
                                                    1,//CORE_INPUT_PORTS,
                                                    CORE_INPUT_CHANNELS/CORE_INPUT_PORTS,
                                                    lowestID);
  localDataNetworks.push_back(localNetwork);

  // Connect things up.
  for(uint i=0; i<INPUT_PORTS_PER_TILE; i++) {
    int outputIndex = (tileID * INPUT_PORTS_PER_TILE) + i;
    localNetwork->dataOut[i](dataOut[outputIndex]);
    localNetwork->canSendData[i](canSendData[outputIndex]);
  }
  for(uint i=0; i<OUTPUT_PORTS_PER_TILE; i++) {
    int inputIndex = (tileID * OUTPUT_PORTS_PER_TILE) + i;
    localNetwork->dataIn[i](dataIn[inputIndex]);
    localNetwork->canReceiveData[i](canReceiveData[inputIndex]);
  }

  localNetwork->clock(clock);

  // Simplify the network if there is only one tile: there is no longer any
  // need for a global network, so this local network can connect directly
  // to the OffChip component.
  if(NUM_TILES == 1) {
    localNetwork->dataIn[OUTPUT_PORTS_PER_TILE](dataFromComponents[0]);
    localNetwork->canReceiveData[OUTPUT_PORTS_PER_TILE](readyForData[0]);
    localNetwork->dataOut[INPUT_PORTS_PER_TILE](dataToComponents[0]);
    localNetwork->canSendData[INPUT_PORTS_PER_TILE](compReadyForData[0]);
//    localNetwork->externalInput()(dataFromComponents[/*TOTAL_OUTPUT_PORTS*/0]);
//    localNetwork->externalOutput()(dataToComponents[/*TOTAL_INPUT_PORTS*/0]);
//    localNetwork->externalReadyInput()(compReadyForData[/*TOTAL_INPUT_PORTS*/0]);
//    localNetwork->externalReadyOutput()(readyForData[/*TOTAL_OUTPUT_PORTS*/0]);
  }

}

void NetworkHierarchy::makeGlobalDataNetwork() {

  // The global network is (currently) connected to a single input and a single
  // output from each tile.
  Arbiter* arbiter = Arbiter::globalDataArbiter(NUM_TILES, NUM_TILES);
  globalDataNetwork = new Crossbar("global_data_net",
                                   0,
                                   0,
                                   TOTAL_INPUT_PORTS,
                                   NUM_TILES,
                                   NUM_TILES,
                                   RoutingComponent::GLOB_DATA,
                                   arbiter);

  globalDataNetwork->clock(clock);

  // Connect the global network to the off-chip component.
  globalDataNetwork->externalInput()(dataFromComponents[/*TOTAL_OUTPUT_PORTS*/0]);
  globalDataNetwork->externalOutput()(dataToComponents[/*TOTAL_INPUT_PORTS*/0]);
  globalDataNetwork->externalReadyInput()(compReadyForData[/*TOTAL_INPUT_PORTS*/0]);
  globalDataNetwork->externalReadyOutput()(readyForData[/*TOTAL_OUTPUT_PORTS*/0]);

  for(uint i=0; i<localDataNetworks.size(); i++) {
//    Network* n = localDataNetworks[i];
//    n->externalInput()(dataToLocalNet[i]);
//    n->externalOutput()(dataFromLocalNet[i]);
//    n->externalReadyInput()(globalReadyForData[i]);
//    n->externalReadyOutput()(localReadyForData[i]);
    NormalCrossbar* n = localDataNetworks[i];
    n->dataIn[OUTPUT_PORTS_PER_TILE](dataToLocalNet[i]);
    n->dataOut[INPUT_PORTS_PER_TILE](dataFromLocalNet[i]);
    n->canSendData[INPUT_PORTS_PER_TILE](globalReadyForData[i]);
    n->canReceiveData[OUTPUT_PORTS_PER_TILE](localReadyForData[i]);
    globalDataNetwork->dataOut[i](dataToLocalNet[i]);
    globalDataNetwork->dataIn[i](dataFromLocalNet[i]);
    globalDataNetwork->canReceiveData[i](globalReadyForData[i]);
    globalDataNetwork->canSendData[i](localReadyForData[i]);
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
  ChannelID lowestID = tileID * OUTPUT_CHANNELS_PER_TILE;
//  ChannelID highestID = lowestID + OUTPUT_CHANNELS_PER_TILE - 1;
//  Arbiter* arbiter = Arbiter::localCreditArbiter(INPUT_PORTS_PER_TILE, OUTPUT_PORTS_PER_TILE);
//  Network* localNetwork = new Crossbar("local_credit_net",
//                                       tileID,
//                                       lowestID,
//                                       highestID,
//                                       INPUT_PORTS_PER_TILE,
//                                       OUTPUT_PORTS_PER_TILE,
//                                       RoutingComponent::LOC_CREDIT,
//                                       arbiter);
  NormalCrossbar* localNetwork = new NormalCrossbar("credit_net",
                                                    tileID,
                                                    INPUT_PORTS_PER_TILE+1,
                                                    OUTPUT_PORTS_PER_TILE+1,
                                                    CORE_OUTPUT_PORTS,
                                                    CORE_OUTPUT_CHANNELS,
                                                    lowestID);
  localCreditNetworks.push_back(localNetwork);

  // Connect things up.
  for(uint i=0; i<OUTPUT_PORTS_PER_TILE; i++) {
    int outputIndex = (tileID * OUTPUT_PORTS_PER_TILE) + i;
    localNetwork->dataOut[i](creditsOut[outputIndex]);
    localNetwork->canSendData[i](canSendCredit[outputIndex]);
  }
  for(uint i=0; i<INPUT_PORTS_PER_TILE; i++) {
    int inputIndex = (tileID * INPUT_PORTS_PER_TILE) + i;
    localNetwork->dataIn[i](creditsIn[inputIndex]);
    localNetwork->canReceiveData[i](canReceiveCredit[inputIndex]);
  }

  localNetwork->clock(clock);

  // Simplify the network if there is only one tile: there is no longer any
  // need for a global network, and this local network can connect directly
  // to the OffChip component.
  if(NUM_TILES == 1) {
//    localNetwork->externalInput()(creditsFromComponents[/*TOTAL_INPUT_PORTS*/0]);
//    localNetwork->externalOutput()(creditsToComponents[/*TOTAL_OUTPUT_PORTS*/0]);
//    localNetwork->externalReadyInput()(compReadyForCredits[/*TOTAL_OUTPUT_PORTS*/0]);
//    localNetwork->externalReadyOutput()(readyForCredits[/*TOTAL_INPUT_PORTS*/0]);
    localNetwork->dataIn[INPUT_PORTS_PER_TILE](creditsFromComponents[0]);
    localNetwork->canReceiveData[INPUT_PORTS_PER_TILE](readyForCredits[0]);
    localNetwork->dataOut[OUTPUT_PORTS_PER_TILE](creditsToComponents[0]);
    localNetwork->canSendData[OUTPUT_PORTS_PER_TILE](compReadyForCredits[0]);
  }

}

void NetworkHierarchy::makeGlobalCreditNetwork() {

  // The global network is (currently) connected to a single input and a single
  // output from each tile.
  Arbiter* arbiter = Arbiter::globalCreditArbiter(NUM_TILES, NUM_TILES);
  globalCreditNetwork = new Crossbar("global_credit_net",
                                     0,
                                     0,
                                     TOTAL_OUTPUT_PORTS,
                                     NUM_TILES,
                                     NUM_TILES,
                                     RoutingComponent::GLOB_CREDIT,
                                     arbiter);

  globalCreditNetwork->clock(clock);

  // Connect the global network to the off-chip component.
  globalCreditNetwork->externalInput()(creditsFromComponents[/*TOTAL_INPUT_PORTS*/0]);
  globalCreditNetwork->externalOutput()(creditsToComponents[/*TOTAL_OUTPUT_PORTS*/0]);
  globalCreditNetwork->externalReadyInput()(compReadyForCredits[/*TOTAL_OUTPUT_PORTS*/0]);
  globalCreditNetwork->externalReadyOutput()(readyForCredits[/*TOTAL_INPUT_PORTS*/0]);

  for(uint i=0; i<localCreditNetworks.size(); i++) {
//    Network* n = localCreditNetworks[i];
//    n->externalInput()(creditsToLocalNet[i]);
//    n->externalOutput()(creditsFromLocalNet[i]);
//    n->externalReadyInput()(globalReadyForCredits[i]);
//    n->externalReadyOutput()(localReadyForCredits[i]);
    NormalCrossbar* n = localCreditNetworks[i];
    n->dataIn[INPUT_PORTS_PER_TILE](creditsToLocalNet[i]);
    n->dataOut[OUTPUT_PORTS_PER_TILE](creditsFromLocalNet[i]);
    n->canSendData[OUTPUT_PORTS_PER_TILE](globalReadyForCredits[i]);
    n->canReceiveData[INPUT_PORTS_PER_TILE](localReadyForCredits[i]);
    globalCreditNetwork->dataOut[i](creditsToLocalNet[i]);
    globalCreditNetwork->dataIn[i](creditsFromLocalNet[i]);
    globalCreditNetwork->canReceiveData[i](globalReadyForCredits[i]);
    globalCreditNetwork->canSendData[i](localReadyForCredits[i]);
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
  canReceiveData        = new ReadyOutput[TOTAL_OUTPUT_PORTS];
  canSendData           = new ReadyInput[TOTAL_INPUT_PORTS];
  canReceiveCredit      = new ReadyOutput[TOTAL_INPUT_PORTS];
  canSendCredit         = new ReadyInput[TOTAL_OUTPUT_PORTS];

  // Make wires. We have one extra wire of each type because we have an
  // additional connection to the off-chip component.
  dataFromComponents    = new DataSignal[/*TOTAL_OUTPUT_PORTS+*/1];
  dataToComponents      = new DataSignal[/*TOTAL_INPUT_PORTS+*/1];
  creditsFromComponents = new CreditSignal[/*TOTAL_INPUT_PORTS+*/1];
  creditsToComponents   = new CreditSignal[/*TOTAL_OUTPUT_PORTS+*/1];
  compReadyForData      = new ReadySignal[/*TOTAL_INPUT_PORTS+*/1];
  compReadyForCredits   = new ReadySignal[/*TOTAL_OUTPUT_PORTS+*/1];
  readyForData          = new ReadySignal[/*TOTAL_OUTPUT_PORTS+*/1];
  readyForCredits       = new ReadySignal[/*TOTAL_INPUT_PORTS+*/1];

  dataFromLocalNet      = new DataSignal[NUM_TILES];
  dataToLocalNet        = new DataSignal[NUM_TILES];
  creditsFromLocalNet   = new CreditSignal[NUM_TILES];
  creditsToLocalNet     = new CreditSignal[NUM_TILES];
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
  delete[] canReceiveData;            delete[] canSendData;
  delete[] canReceiveCredit;          delete[] canSendCredit;

  delete[] dataFromComponents;        delete[] dataToComponents;
  delete[] creditsFromComponents;     delete[] creditsToComponents;
  delete[] compReadyForData;          delete[] readyForData;
  delete[] compReadyForCredits;       delete[] readyForCredits;

  delete[] dataToLocalNet;            delete[] dataFromLocalNet;
  delete[] creditsToLocalNet;         delete[] creditsFromLocalNet;
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
