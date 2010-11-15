/*
 * NetworkHierarchy.cpp
 *
 *  Created on: 1 Nov 2010
 *      Author: db434
 */

#include "NetworkHierarchy.h"
#include "Network.h"
#include "Topologies/Crossbar.h"
#include "FlowControl2/FlowControlIn.h"
#include "FlowControl2/FlowControlOut.h"

void NetworkHierarchy::setupFlowControl() {

  // Create all FlowControlIns.
  for(uint input=0; input<TOTAL_INPUTS; input++) {
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
  for(uint output=0; output<TOTAL_OUTPUTS; output++) {
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
  FlowControlIn* fcin = new FlowControlIn("fc_in", TOTAL_INPUTS);
  flowControlIn.push_back(fcin);
  FlowControlOut* fcout = new FlowControlOut("fc_out", TOTAL_OUTPUTS);
  flowControlOut.push_back(fcout);

  fcin->dataOut(dataToOffChip);            offChip.dataIn(dataToOffChip);
  fcin->flowControlIn(creditsFromOffChip); offChip.creditsOut(creditsFromOffChip);
  fcin->dataIn(dataToComponents[TOTAL_INPUTS]);
  fcin->readyOut(compReadyForData[TOTAL_INPUTS]);
  fcin->creditsOut(creditsFromComponents[TOTAL_INPUTS]);
  fcin->readyIn(readyForCredits[TOTAL_INPUTS]);

  fcout->dataIn(dataFromOffChip);          offChip.dataOut(dataFromOffChip);
  fcout->flowControlOut(readyToOffChip);   offChip.readyIn(readyToOffChip);
  fcout->dataOut(dataFromComponents[TOTAL_OUTPUTS]);
  fcout->readyIn(readyForData[TOTAL_OUTPUTS]);
  fcout->creditsIn(creditsToComponents[TOTAL_OUTPUTS]);
  fcout->readyOut(compReadyForCredits[TOTAL_OUTPUTS]);

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

  // The total numbers of input/output ports in a tile.
  static int numTileInputs  = COMPONENTS_PER_TILE * NUM_CLUSTER_INPUTS;
  static int numTileOutputs = COMPONENTS_PER_TILE * NUM_CLUSTER_OUTPUTS;

  // Create a local network.
  ChannelID lowestID = tileID * numTileInputs;
  ChannelID highestID = lowestID + numTileInputs - 1;
  Network* localNetwork = new Crossbar("local_data_net", tileID, lowestID, highestID,
                                       numTileOutputs, numTileInputs);
  localDataNetworks.push_back(localNetwork);

  // Connect things up.
  for(int i=0; i<numTileInputs; i++) {
    int outputIndex = (tileID * numTileInputs) + i;
    localNetwork->dataOut[i](dataToComponents[outputIndex]);
    localNetwork->readyIn[i](compReadyForData[outputIndex]);
  }
  for(int i=0; i<numTileOutputs; i++) {
    int inputIndex = (tileID * numTileOutputs) + i;
    localNetwork->dataIn[i](dataFromComponents[inputIndex]);
    localNetwork->readyOut[i](readyForData[inputIndex]);
  }

  localNetwork->clock(clock);

  // Simplify the network if there is only one tile: there is no longer any
  // need for a global network, so this local network can connect directly
  // to the OffChip component.
  if(NUM_TILES == 1) {
    localNetwork->externalInput()(dataFromComponents[TOTAL_OUTPUTS]);
    localNetwork->externalOutput()(dataToComponents[TOTAL_INPUTS]);
    localNetwork->externalReadyInput()(compReadyForData[TOTAL_INPUTS]);
    localNetwork->externalReadyOutput()(readyForData[TOTAL_OUTPUTS]);
  }

}

void NetworkHierarchy::makeGlobalDataNetwork() {

  // The global network is (currently) connected to a single input and a single
  // output from each tile.
  globalDataNetwork = new Crossbar("global_data_net", 0, 0, TOTAL_INPUTS,
                                   NUM_TILES, NUM_TILES);

  globalDataNetwork->clock(clock);

  // Connect the global network to the off-chip component.
  globalDataNetwork->externalInput()(dataFromComponents[TOTAL_OUTPUTS]);
  globalDataNetwork->externalOutput()(dataToComponents[TOTAL_INPUTS]);
  globalDataNetwork->externalReadyInput()(compReadyForData[TOTAL_INPUTS]);
  globalDataNetwork->externalReadyOutput()(readyForData[TOTAL_OUTPUTS]);

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

  // The total numbers of input/output ports in a tile. Input and output
  // numbers are reversed because credits are sent to output ports.
  static int numTileInputs  = COMPONENTS_PER_TILE * NUM_CLUSTER_INPUTS;
  static int numTileOutputs = COMPONENTS_PER_TILE * NUM_CLUSTER_OUTPUTS;

  // Create a local network.
  ChannelID lowestID = tileID * numTileOutputs;
  ChannelID highestID = lowestID + numTileOutputs - 1;
  Network* localNetwork = new Crossbar("local_credit_net", tileID, lowestID, highestID,
                                       numTileInputs, numTileOutputs);
  localCreditNetworks.push_back(localNetwork);

  // Connect things up.
  for(int i=0; i<numTileOutputs; i++) {
    int outputIndex = (tileID * numTileOutputs) + i;
    localNetwork->dataOut[i](creditsToComponents[outputIndex]);
    localNetwork->readyIn[i](compReadyForCredits[outputIndex]);
  }
  for(int i=0; i<numTileInputs; i++) {
    int inputIndex = (tileID * numTileInputs) + i;
    localNetwork->dataIn[i](creditsFromComponents[inputIndex]);
    localNetwork->readyOut[i](readyForCredits[inputIndex]);
  }

  localNetwork->clock(clock);

  // Simplify the network if there is only one tile: there is no longer any
  // need for a global network, and this local network can connect directly
  // to the OffChip component.
  if(NUM_TILES == 1) {
    localNetwork->externalInput()(creditsFromComponents[TOTAL_INPUTS]);
    localNetwork->externalOutput()(creditsToComponents[TOTAL_OUTPUTS]);
    localNetwork->externalReadyInput()(compReadyForCredits[TOTAL_OUTPUTS]);
    localNetwork->externalReadyOutput()(readyForCredits[TOTAL_INPUTS]);
  }

}

void NetworkHierarchy::makeGlobalCreditNetwork() {

  // The global network is (currently) connected to a single input and a single
  // output from each tile.
  globalCreditNetwork = new Crossbar("global_credit_net", 0, 0, TOTAL_OUTPUTS,
                                     NUM_TILES, NUM_TILES);

  globalCreditNetwork->clock(clock);

  // Connect the global network to the off-chip component.
  globalCreditNetwork->externalInput()(creditsFromComponents[TOTAL_INPUTS]);
  globalCreditNetwork->externalOutput()(creditsToComponents[TOTAL_OUTPUTS]);
  globalCreditNetwork->externalReadyInput()(compReadyForCredits[TOTAL_OUTPUTS]);
  globalCreditNetwork->externalReadyOutput()(readyForCredits[TOTAL_INPUTS]);

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

std::string NetworkHierarchy::portLocation(ChannelID port, bool isInput) {
  int numPorts = isInput ? NUM_CLUSTER_INPUTS : NUM_CLUSTER_OUTPUTS;
  std::stringstream ss;
  ss << "(" << (port/numPorts) << "," << (port%numPorts) << ")";
  std::string result;
  ss >> result;
  return result;
}

NetworkHierarchy::NetworkHierarchy(sc_module_name name) :
    Component(name),
    offChip("offchip") {

  // Make ports.
  dataIn    = new sc_in<DataType>[TOTAL_OUTPUTS];
  dataOut   = new sc_out<Word>[TOTAL_INPUTS];
  creditsIn = new sc_in<int>[TOTAL_INPUTS];
  readyOut  = new sc_out<bool>[TOTAL_OUTPUTS];

  // Make wires. We have one extra wire of each type because we have an
  // additional connection to the off-chip component.
  dataFromComponents    = new DataSignal[TOTAL_OUTPUTS+1];
  dataToComponents      = new DataSignal[TOTAL_INPUTS+1];
  creditsFromComponents = new CreditSignal[TOTAL_INPUTS+1];
  creditsToComponents   = new CreditSignal[TOTAL_OUTPUTS+1];
  compReadyForData      = new sc_signal<bool>[TOTAL_INPUTS+1];
  compReadyForCredits   = new sc_signal<bool>[TOTAL_OUTPUTS+1];
  readyForData          = new sc_signal<bool>[TOTAL_OUTPUTS+1];
  readyForCredits       = new sc_signal<bool>[TOTAL_INPUTS+1];

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
