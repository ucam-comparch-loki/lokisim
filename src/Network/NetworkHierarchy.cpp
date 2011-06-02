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
#include "Topologies/LocalNetwork.h"

void NetworkHierarchy::setupFlowControl() {

  // All cores and memories are now responsible for their own flow control.

  // Attach flow control units to the off-chip component too.
  FlowControlIn*  fcin  = new FlowControlIn("fc_in", ComponentID()/*TOTAL_INPUT_PORTS*/, ChannelID());
  flowControlIn.push_back(fcin);
  FlowControlOut* fcout = new FlowControlOut("fc_out", ComponentID()/*TOTAL_OUTPUT_PORTS*/, ChannelID());
  flowControlOut.push_back(fcout);

  fcin->clock(clock);
  fcin->dataOut(dataToOffChip);            offChip.dataIn(dataToOffChip);
  fcin->bufferHasSpace(readyFromOffChip);  offChip.readyOut(readyFromOffChip);
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

void NetworkHierarchy::makeLocalNetwork(int tileID) {

  // Create a local network.
  LocalNetwork* localNetwork = new LocalNetwork(sc_gen_unique_name("tile_net"), ComponentID(tileID, 0));
  localNetworks.push_back(localNetwork);

  // Connect things up.
  localNetwork->clock(clock);

  for(unsigned int i=0; i<INPUT_PORTS_PER_TILE; i++) {
    int outputIndex = (tileID * INPUT_PORTS_PER_TILE) + i;
    localNetwork->dataOut[i](dataOut[outputIndex]);
    localNetwork->validDataOut[i](validDataOut[outputIndex]);
    localNetwork->ackDataOut[i](ackDataOut[outputIndex]);

    localNetwork->creditsIn[i](creditsIn[outputIndex]);
    localNetwork->validCreditIn[i](validCreditIn[outputIndex]);
    localNetwork->ackCreditIn[i](ackCreditIn[outputIndex]);
  }

  for(unsigned int i=0; i<OUTPUT_PORTS_PER_TILE; i++) {
    int inputIndex = (tileID * OUTPUT_PORTS_PER_TILE) + i;
    localNetwork->dataIn[i](dataIn[inputIndex]);
    localNetwork->validDataIn[i](validDataIn[inputIndex]);
    localNetwork->ackDataIn[i](ackDataIn[inputIndex]);

    localNetwork->creditsOut[i](creditsOut[inputIndex]);
    localNetwork->validCreditOut[i](validCreditOut[inputIndex]);
    localNetwork->ackCreditOut[i](ackCreditOut[inputIndex]);
  }

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

    localNetwork->externalCreditIn()(creditsFromComponents[0]);
    localNetwork->externalValidCreditIn()(validCreditFromComps[0]);
    localNetwork->externalAckCreditOut()(ackCreditFromComps[0]);
    localNetwork->externalCreditOut()(creditsToComponents[0]);
    localNetwork->externalValidCreditOut()(validCreditToComps[0]);
    localNetwork->externalAckCreditIn()(ackCreditToComps[0]);
  }
}

void NetworkHierarchy::makeGlobalNetwork() {

  // Make data network.
  globalDataNetwork = new Crossbar("global_data_net",
                                   0,
                                   NUM_TILES,
                                   NUM_TILES,
                                   1,
                                   Network::TILE,   // This network connects tiles
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

  for(uint i=0; i<localNetworks.size(); i++) {
    Network* n = localNetworks[i];
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

  // Make credit network.
  // TODO: integrate both networks into one GlobalNetwork.
  globalDataNetwork = new Crossbar("global_credit_net",
                                   0,
                                   NUM_TILES,
                                   NUM_TILES,
                                   1,
                                   Network::TILE,   // This network connects tiles
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

  for(uint i=0; i<localNetworks.size(); i++) {
    Network* n = localNetworks[i];
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
  for(uint i=0; i<localNetworks.size(); i++) delete localNetworks[i];

  // The global networks won't exist if there is only one tile, so we can't
  // delete them.
  if(NUM_TILES>1) {
    delete globalDataNetwork;
    delete globalCreditNetwork;
  }
}
