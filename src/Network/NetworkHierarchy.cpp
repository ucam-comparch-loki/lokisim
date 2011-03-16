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

void NetworkHierarchy::setupFlowControl() {

  // Create all FlowControlIns.
  for(uint input=0; input<TOTAL_INPUT_PORTS; input++) {
    FlowControlIn* fc = new FlowControlIn("fc_in", input);
    flowControlIn.push_back(fc);

    // Bind to network's inputs/outputs.
    fc->flowControlIn(creditsIn[input]);
    fc->dataOut(dataOut[input]);

    // Connect up internal signals.
    fc->dataIn(dataToComponents[input]);
    fc->readyOut(compReadyForData[input]);
    fc->creditsOut(creditsFromComponents[input]);
    fc->readyIn(readyForCredits[input]);
  }

  // Create all FlowControlOuts.
  for(uint output=0; output<TOTAL_OUTPUT_PORTS; output++) {
    FlowControlOut* fc = new FlowControlOut("fc_out", output);
    flowControlOut.push_back(fc);

    // Bind to network's inputs/outputs.
    fc->dataIn(dataIn[output]);
    fc->flowControlOut(readyOut[output]);

    // Connect up internal signals.
    fc->dataOut(dataFromComponents[output]);
    fc->readyIn(readyForData[output]);
    fc->creditsIn(creditsToComponents[output]);
    fc->readyOut(compReadyForCredits[output]);
  }

  // Attach flow control units to the off-chip component too.
  FlowControlIn* fcin = new FlowControlIn("fc_in", TOTAL_INPUT_PORTS);
  flowControlIn.push_back(fcin);
  FlowControlOut* fcout = new FlowControlOut("fc_out", TOTAL_OUTPUT_PORTS);
  flowControlOut.push_back(fcout);

  fcin->dataOut(dataToOffChip);            offChip.dataIn(dataToOffChip);
  fcin->flowControlIn(creditsFromOffChip); offChip.creditsOut(creditsFromOffChip);
  fcin->dataIn(dataToComponents[TOTAL_INPUT_PORTS]);
  fcin->readyOut(compReadyForData[TOTAL_INPUT_PORTS]);
  fcin->creditsOut(creditsFromComponents[TOTAL_INPUT_PORTS]);
  fcin->readyIn(readyForCredits[TOTAL_INPUT_PORTS]);

  fcout->dataIn(dataFromOffChip);          offChip.dataOut(dataFromOffChip);
  fcout->flowControlOut(readyToOffChip);   offChip.readyIn(readyToOffChip);
  fcout->dataOut(dataFromComponents[TOTAL_OUTPUT_PORTS]);
  fcout->readyIn(readyForData[TOTAL_OUTPUT_PORTS]);
  fcout->creditsIn(creditsToComponents[TOTAL_OUTPUT_PORTS]);
  fcout->readyOut(compReadyForCredits[TOTAL_OUTPUT_PORTS]);

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
  ChannelID highestID = lowestID + INPUT_CHANNELS_PER_TILE - 1;
  Arbiter* arbiter = Arbiter::localDataArbiter(OUTPUT_PORTS_PER_TILE, INPUT_PORTS_PER_TILE);
  Network* localNetwork = new Crossbar("local_data_net",
                                       tileID,
                                       lowestID,
                                       highestID,
                                       OUTPUT_PORTS_PER_TILE,
                                       INPUT_PORTS_PER_TILE,
                                       RoutingComponent::LOC_DATA,
                                       arbiter);
  localDataNetworks.push_back(localNetwork);

  // Connect things up.
  for(uint i=0; i<INPUT_PORTS_PER_TILE; i++) {
    int outputIndex = (tileID * INPUT_PORTS_PER_TILE) + i;
    localNetwork->dataOut[i](dataToComponents[outputIndex]);
    localNetwork->readyIn[i](compReadyForData[outputIndex]);
  }
  for(uint i=0; i<OUTPUT_PORTS_PER_TILE; i++) {
    int inputIndex = (tileID * OUTPUT_PORTS_PER_TILE) + i;
    localNetwork->dataIn[i](dataFromComponents[inputIndex]);
    localNetwork->readyOut[i](readyForData[inputIndex]);
  }

  localNetwork->clock(clock);

  // Simplify the network if there is only one tile: there is no longer any
  // need for a global network, so this local network can connect directly
  // to the OffChip component.
  if(NUM_TILES == 1) {
    localNetwork->externalInput()(dataFromComponents[TOTAL_OUTPUT_PORTS]);
    localNetwork->externalOutput()(dataToComponents[TOTAL_INPUT_PORTS]);
    localNetwork->externalReadyInput()(compReadyForData[TOTAL_INPUT_PORTS]);
    localNetwork->externalReadyOutput()(readyForData[TOTAL_OUTPUT_PORTS]);
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
  globalDataNetwork->externalInput()(dataFromComponents[TOTAL_OUTPUT_PORTS]);
  globalDataNetwork->externalOutput()(dataToComponents[TOTAL_INPUT_PORTS]);
  globalDataNetwork->externalReadyInput()(compReadyForData[TOTAL_INPUT_PORTS]);
  globalDataNetwork->externalReadyOutput()(readyForData[TOTAL_OUTPUT_PORTS]);

  for(uint i=0; i<localDataNetworks.size(); i++) {
    Network* n = localDataNetworks[i];
    n->externalInput()(dataToLocalNet[i]);
    n->externalOutput()(dataFromLocalNet[i]);
    n->externalReadyInput()(globalReadyForData[i]);
    n->externalReadyOutput()(localReadyForData[i]);
    globalDataNetwork->dataOut[i](dataToLocalNet[i]);
    globalDataNetwork->dataIn[i](dataFromLocalNet[i]);
    globalDataNetwork->readyOut[i](globalReadyForData[i]);
    globalDataNetwork->readyIn[i](localReadyForData[i]);
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
  ChannelID lowestID = tileID * OUTPUT_PORTS_PER_TILE;  // TODO: CHANNELS
  ChannelID highestID = lowestID + OUTPUT_PORTS_PER_TILE - 1; // TODO: CHANNELS
  Arbiter* arbiter = Arbiter::localCreditArbiter(INPUT_PORTS_PER_TILE, OUTPUT_PORTS_PER_TILE);
  Network* localNetwork = new Crossbar("local_credit_net",
                                       tileID,
                                       lowestID,
                                       highestID,
                                       INPUT_PORTS_PER_TILE,
                                       OUTPUT_PORTS_PER_TILE,
                                       RoutingComponent::LOC_CREDIT,
                                       arbiter);
  localCreditNetworks.push_back(localNetwork);

  // Connect things up.
  for(uint i=0; i<OUTPUT_PORTS_PER_TILE; i++) {
    int outputIndex = (tileID * OUTPUT_PORTS_PER_TILE) + i;
    localNetwork->dataOut[i](creditsToComponents[outputIndex]);
    localNetwork->readyIn[i](compReadyForCredits[outputIndex]);
  }
  for(uint i=0; i<INPUT_PORTS_PER_TILE; i++) {
    int inputIndex = (tileID * INPUT_PORTS_PER_TILE) + i;
    localNetwork->dataIn[i](creditsFromComponents[inputIndex]);
    localNetwork->readyOut[i](readyForCredits[inputIndex]);
  }

  localNetwork->clock(clock);

  // Simplify the network if there is only one tile: there is no longer any
  // need for a global network, and this local network can connect directly
  // to the OffChip component.
  if(NUM_TILES == 1) {
    localNetwork->externalInput()(creditsFromComponents[TOTAL_INPUT_PORTS]);
    localNetwork->externalOutput()(creditsToComponents[TOTAL_OUTPUT_PORTS]);
    localNetwork->externalReadyInput()(compReadyForCredits[TOTAL_OUTPUT_PORTS]);
    localNetwork->externalReadyOutput()(readyForCredits[TOTAL_INPUT_PORTS]);
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
  globalCreditNetwork->externalInput()(creditsFromComponents[TOTAL_INPUT_PORTS]);
  globalCreditNetwork->externalOutput()(creditsToComponents[TOTAL_OUTPUT_PORTS]);
  globalCreditNetwork->externalReadyInput()(compReadyForCredits[TOTAL_OUTPUT_PORTS]);
  globalCreditNetwork->externalReadyOutput()(readyForCredits[TOTAL_INPUT_PORTS]);

  for(uint i=0; i<localCreditNetworks.size(); i++) {
    Network* n = localCreditNetworks[i];
    n->externalInput()(creditsToLocalNet[i]);
    n->externalOutput()(creditsFromLocalNet[i]);
    n->externalReadyInput()(globalReadyForCredits[i]);
    n->externalReadyOutput()(localReadyForCredits[i]);
    globalCreditNetwork->dataOut[i](creditsToLocalNet[i]);
    globalCreditNetwork->dataIn[i](creditsFromLocalNet[i]);
    globalCreditNetwork->readyOut[i](globalReadyForCredits[i]);
    globalCreditNetwork->readyIn[i](localReadyForCredits[i]);
  }

}

NetworkHierarchy::NetworkHierarchy(sc_module_name name) :
    Component(name),
    offChip("offchip") {

  // Make ports.
  dataIn    = new sc_in<DataType>[TOTAL_OUTPUT_PORTS];
  dataOut   = new sc_out<Word>[TOTAL_INPUT_PORTS];
  creditsIn = new sc_in<int>[TOTAL_INPUT_PORTS];
  readyOut = new sc_out<bool>[TOTAL_OUTPUT_PORTS];

  // Make wires. We have one extra wire of each type because we have an
  // additional connection to the off-chip component.
  dataFromComponents    = new DataSignal[TOTAL_OUTPUT_PORTS+1];
  dataToComponents      = new DataSignal[TOTAL_INPUT_PORTS+1];
  creditsFromComponents = new CreditSignal[TOTAL_INPUT_PORTS+1];
  creditsToComponents   = new CreditSignal[TOTAL_OUTPUT_PORTS+1];
  compReadyForData      = new sc_signal<bool>[TOTAL_INPUT_PORTS+1];
  compReadyForCredits   = new sc_signal<bool>[TOTAL_OUTPUT_PORTS+1];
  readyForData          = new sc_signal<bool>[TOTAL_OUTPUT_PORTS+1];
  readyForCredits       = new sc_signal<bool>[TOTAL_INPUT_PORTS+1];

  dataFromLocalNet      = new DataSignal[NUM_TILES];
  dataToLocalNet        = new DataSignal[NUM_TILES];
  creditsFromLocalNet   = new CreditSignal[NUM_TILES];
  creditsToLocalNet     = new CreditSignal[NUM_TILES];
  localReadyForData     = new sc_signal<bool>[NUM_TILES];
  localReadyForCredits  = new sc_signal<bool>[NUM_TILES];
  globalReadyForData    = new sc_signal<bool>[NUM_TILES];
  globalReadyForCredits = new sc_signal<bool>[NUM_TILES];

  // Make networks.
  setupFlowControl();
  makeDataNetwork();
  makeCreditNetwork();

}

NetworkHierarchy::~NetworkHierarchy() {
  delete[] dataIn;                    delete[] dataOut;
  delete[] creditsIn;                 delete[] readyOut;

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
