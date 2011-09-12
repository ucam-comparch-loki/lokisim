/*
 * NewLocalNetwork.cpp
 *
 *  Created on: 9 Sep 2011
 *      Author: db434
 */

#include "NewLocalNetwork.h"
#include "../Multiplexer.h"
#include "../Arbiters/EndArbiter.h"

// Only cores and the global network can send/receive credits.
const unsigned int NewLocalNetwork::creditInputs  = CORES_PER_TILE + 1;
const unsigned int NewLocalNetwork::creditOutputs = CORES_PER_TILE + 1;

// 2 inputs each from core-to-core and memory-to-core networks, plus one
// from the router.
const unsigned int NewLocalNetwork::muxInputs = CORE_INPUT_PORTS + CORE_INPUT_PORTS + 1;

const sc_event& NewLocalNetwork::makeRequest(ComponentID source,
                                             ChannelID destination,
                                             bool request) {
  // Find out which signal to write the request to (and read the grant from).
  sc_signal<bool> *requestSignal, *grantSignal;

  if(source.isMemory()) {                             // Memory to core
    assert(destination.isCore());
    requestSignal = &coreRequests[source.getPosition()][destination.getPosition()];
    grantSignal   = &coreGrants[source.getPosition()][destination.getPosition()];
  }
  else if(source.getTile() != id.getTile()) {         // Global to core
    assert(destination.isCore());
    requestSignal = &coreRequests[COMPONENTS_PER_TILE][destination.getPosition()];
    grantSignal   = &coreGrants[COMPONENTS_PER_TILE][destination.getPosition()];
  }
  else {
    if(destination.isMemory()) {                      // Core to memory
      requestSignal = &memRequests[source.getPosition()][destination.getPosition()-CORES_PER_TILE];
      grantSignal   = &memGrants[source.getPosition()][destination.getPosition()-CORES_PER_TILE];
    }
    else if(destination.getTile() != id.getTile()) {  // Core to global
      requestSignal = &globalRequests[source.getPosition()][0];
      grantSignal   = &globalGrants[source.getPosition()][0];
    }
    else {                                            // Core to core
      requestSignal = &coreRequests[source.getPosition()][destination.getPosition()];
      grantSignal   = &coreGrants[source.getPosition()][destination.getPosition()];
    }
  }

  // Send the request.
  requestSignal->write(request);

  // Return an event which will be triggered when the request is granted (if
  // a request was made). If no request was made, this event is not meaningful,
  // and can be ignored.
  return grantSignal->posedge_event();
}

void NewLocalNetwork::createArbiters() {
  for(unsigned int i=0; i<CORES_PER_TILE; i++) {
    EndArbiter* arb = new EndArbiter(sc_gen_unique_name("arbiter"),
                                     id,
                                     muxInputs,
                                     CORE_INPUT_PORTS,
                                     true);

    // Is this right? Does the arbiter need to have its clock skewed at all?
    arb->clock(clock);

    for(int j=0; j<arb->numInputs(); j++) {
      arb->requests[j](requestSig[i][j]);
      arb->grants[j](grantSig[i][j]);
    }
    for(int j=0; j<arb->numOutputs(); j++) {
      arb->select[j](selectSig[i][j]);
    }

    arbiters.push_back(arb);
  }
}

void NewLocalNetwork::createMuxes() {
  for(unsigned int i=0; i<CORES_PER_TILE*CORE_INPUT_PORTS; i++) {
    Multiplexer* mux = new Multiplexer(sc_gen_unique_name("mux"), muxInputs);

    // Since each arbiter may control multiple multiplexers, determine which
    // arbiter is controlling this one.
    int arbiter = i/CORE_INPUT_PORTS;
    int arbiterSelection = i % CORE_INPUT_PORTS;
    mux->select(selectSig[arbiter][arbiterSelection]);

    for(int j=0; j<mux->inputs(); j++) {
      // Use "arbiter" rather than "i" as the index because the same data go to
      // all muxes leading to the same component. All are controlled by 1 arbiter.
      mux->dataIn[j](dataSig[arbiter][j]);
      mux->validIn[j](validSig[arbiter][j]);
      mux->ackIn[j](ackSig[arbiter][j]);
    }

    mux->dataOut(dataOut[i]);
    mux->validOut(validDataOut[i]);
    mux->ackOut(ackDataOut[i]);

    muxes.push_back(mux);
  }
}

void NewLocalNetwork::createSignals() {
  dataSig    = new DataSignal*[CORES_PER_TILE];
  validSig   = new ReadySignal*[CORES_PER_TILE];
  ackSig     = new ReadySignal*[CORES_PER_TILE];
  selectSig  = new sc_signal<int>*[CORES_PER_TILE];
  requestSig = new sc_signal<bool>*[CORES_PER_TILE];
  grantSig   = new sc_signal<bool>*[CORES_PER_TILE];

  for(unsigned int i=0; i<CORES_PER_TILE; i++) {
    dataSig[i]    = new DataSignal[muxInputs];
    validSig[i]   = new ReadySignal[muxInputs];
    ackSig[i]     = new ReadySignal[muxInputs];

    selectSig[i]  = new sc_signal<int>[CORE_INPUT_PORTS];
    requestSig[i] = new sc_signal<bool>[muxInputs];
    grantSig[i]   = new sc_signal<bool>[muxInputs];
  }

  int numCores = CORES_PER_TILE;
  int sendToCores = CORES_PER_TILE + MEMS_PER_TILE + 1;
  coreRequests = new sc_signal<bool>*[sendToCores];
  coreGrants   = new sc_signal<bool>*[sendToCores];
  for(int i=0; i<sendToCores; i++) {
    coreRequests[i] = new sc_signal<bool>[numCores];
    coreGrants[i]   = new sc_signal<bool>[numCores];
  }

  int numMems = MEMS_PER_TILE;
  int sendToMems = CORES_PER_TILE;
  memRequests = new sc_signal<bool>*[sendToMems];
  memGrants   = new sc_signal<bool>*[sendToMems];
  for(int i=0; i<sendToMems; i++) {
    memRequests[i] = new sc_signal<bool>[numMems];
    memGrants[i]   = new sc_signal<bool>[numMems];
  }

  int numRouterLinks = 1;
  int sendToRouters = CORES_PER_TILE;
  globalRequests = new sc_signal<bool>*[sendToRouters];
  globalGrants   = new sc_signal<bool>*[sendToRouters];
  for(int i=0; i<sendToRouters; i++) {
    globalRequests[i] = new sc_signal<bool>[numRouterLinks];
    globalGrants[i]   = new sc_signal<bool>[numRouterLinks];
  }
}

void NewLocalNetwork::wireUpSubnetworks() {
  coreToCore.clock(clock); coreToMemory.clock(clock); coreToGlobal.clock(clock);
  memoryToCore.clock(clock); globalToCore.clock(clock);
  c2gCredits.clock(clock); g2cCredits.clock(clock);

  // Keep track of how many ports in each array have been bound, so that if
  // multiple networks connect to the same array, they can start where the
  // previous network finished.
  int portsBound = 0;

  // Data networks
  for(unsigned int i=0; i<coreToCore.numInputPorts(); i++) {
    // Data from cores can go to all three core-to-X networks. In practice,
    // there might be a demux to reduce switching.
    coreToCore.dataIn[i](dataIn[portsBound]);
    coreToCore.validDataIn[i](validDataIn[portsBound]);
    coreToCore.ackDataIn[i](ackDataIn[portsBound]);
    coreToMemory.dataIn[i](dataIn[portsBound]);
    coreToMemory.validDataIn[i](validDataIn[portsBound]);
    coreToMemory.ackDataIn[i](ackDataIn[portsBound]);
    coreToGlobal.dataIn[i](dataIn[portsBound]);
    coreToGlobal.validDataIn[i](validDataIn[portsBound]);
    coreToGlobal.ackDataIn[i](ackDataIn[portsBound]);

    portsBound++;
  }
  for(unsigned int i=0; i<memoryToCore.numInputPorts(); i++) {
    memoryToCore.dataIn[i](dataIn[portsBound]);
    memoryToCore.validDataIn[i](validDataIn[portsBound]);
    memoryToCore.ackDataIn[i](ackDataIn[portsBound]);
    portsBound++;
  }
  for(unsigned int i=0; i<globalToCore.numInputPorts(); i++) {
    globalToCore.dataIn[i](dataIn[portsBound]);
    globalToCore.validDataIn[i](validDataIn[portsBound]);
    globalToCore.ackDataIn[i](ackDataIn[portsBound]);
    portsBound++;
  }

  for(unsigned int i=0; i<coreToCore.numOutputPorts(); i++) {
    int arbiter = i/CORE_INPUT_PORTS;
    int arbiterInput = i%CORE_INPUT_PORTS;
    coreToCore.dataOut[i](dataSig[arbiter][arbiterInput]);
    coreToCore.validDataOut[i](validSig[arbiter][arbiterInput]);
    coreToCore.ackDataOut[i](ackSig[arbiter][arbiterInput]);

    int arbiterOutput = i % CORE_INPUT_PORTS;
    coreToCore.requestsOut[arbiter][arbiterOutput](requestSig[arbiter][arbiterInput]);
    coreToCore.grantsIn[arbiter][arbiterOutput](grantSig[arbiter][arbiterInput]);
  }
  for(unsigned int i=0; i<memoryToCore.numOutputPorts(); i++) {
    int arbiter = i/CORE_INPUT_PORTS;
    int arbiterInput = (i%CORE_INPUT_PORTS) + CORE_INPUT_PORTS;
    memoryToCore.dataOut[i](dataSig[arbiter][arbiterInput]);
    memoryToCore.validDataOut[i](validSig[arbiter][arbiterInput]);
    memoryToCore.ackDataOut[i](ackSig[arbiter][arbiterInput]);

    int arbiterOutput = i % CORE_INPUT_PORTS;
    memoryToCore.requestsOut[arbiter][arbiterOutput](requestSig[arbiter][arbiterInput]);
    memoryToCore.grantsIn[arbiter][arbiterOutput](grantSig[arbiter][arbiterInput]);
  }
  for(unsigned int i=0; i<globalToCore.numOutputPorts(); i++) {
    int arbiter = i;  // Only one output per core
    int arbiterInput = CORE_INPUT_PORTS + CORE_INPUT_PORTS;
    globalToCore.dataOut[i](dataSig[arbiter][arbiterInput]);
    globalToCore.validDataOut[i](validSig[arbiter][arbiterInput]);
    globalToCore.ackDataOut[i](ackSig[arbiter][arbiterInput]);

    int arbiterOutput = 0;
    globalToCore.requestsOut[arbiter][arbiterOutput](requestSig[arbiter][arbiterInput]);
    globalToCore.grantsIn[arbiter][arbiterOutput](grantSig[arbiter][arbiterInput]);
  }
  for(unsigned int i=0; i<coreToMemory.numOutputPorts(); i++) {
    // Some ports will have already been connected.
    int port = i + (CORES_PER_TILE*CORE_INPUT_PORTS);
    coreToMemory.dataOut[i](dataOut[port]);
    coreToMemory.validDataOut[i](validDataOut[port]);
    coreToMemory.ackDataOut[i](ackDataOut[port]);
  }
  for(unsigned int i=0; i<coreToGlobal.numOutputPorts(); i++) {
    // Some ports will have already been connected.
    int port = i + (CORES_PER_TILE*CORE_INPUT_PORTS)
                 + (MEMS_PER_TILE*MEMORY_INPUT_PORTS);
    coreToGlobal.dataOut[i](dataOut[port]);
    coreToGlobal.validDataOut[i](validDataOut[port]);
    coreToGlobal.ackDataOut[i](ackDataOut[port]);
  }

  // Credit networks
  portsBound = 0;
  for(unsigned int i=0; i<c2gCredits.numInputPorts(); i++) {
    c2gCredits.dataIn[i](creditsIn[portsBound]);
    c2gCredits.validDataIn[i](validCreditIn[portsBound]);
    c2gCredits.ackDataIn[i](ackCreditIn[portsBound]);
    portsBound++;
  }
  for(unsigned int i=0; i<g2cCredits.numInputPorts(); i++) {
    g2cCredits.dataIn[i](creditsIn[portsBound]);
    g2cCredits.validDataIn[i](validCreditIn[portsBound]);
    g2cCredits.ackDataIn[i](ackCreditIn[portsBound]);
    portsBound++;
  }

  portsBound = 0;
  for(unsigned int i=0; i<g2cCredits.numOutputPorts(); i++) {
    g2cCredits.dataOut[i](creditsOut[portsBound]);
    g2cCredits.validDataOut[i](validCreditOut[portsBound]);
    g2cCredits.ackDataOut[i](ackCreditOut[portsBound]);
    portsBound++;
  }
  for(unsigned int i=0; i<c2gCredits.numOutputPorts(); i++) {
    c2gCredits.dataOut[i](creditsOut[portsBound]);
    c2gCredits.validDataOut[i](validCreditOut[portsBound]);
    c2gCredits.ackDataOut[i](ackCreditOut[portsBound]);
    portsBound++;
  }

  // Requests/grants for arbitration.
  for(unsigned int input=0; input<CORES_PER_TILE; input++) {
    for(unsigned int output=0; output<CORES_PER_TILE; output++) {
      coreToCore.requestsIn[input][output](coreRequests[input][output]);
      coreToCore.grantsOut[input][output](coreGrants[input][output]);
    }
    for(unsigned int output=0; output<MEMS_PER_TILE; output++) {
      coreToMemory.requestsIn[input][output](memRequests[input][output]);
      coreToMemory.grantsOut[input][output](memGrants[input][output]);
    }
    coreToGlobal.requestsIn[input][0](globalRequests[input][0]);
    coreToGlobal.grantsOut[input][0](globalGrants[input][0]);
  }
  for(unsigned int i=0; i<MEMS_PER_TILE; i++) {
    int input = i + CORES_PER_TILE; // Memories start at offset CORES_PER_TILE
    for(unsigned int output=0; output<CORES_PER_TILE; output++) {
      memoryToCore.requestsIn[i][output](coreRequests[input][output]);
      memoryToCore.grantsOut[i][output](coreGrants[input][output]);
    }
  }
  int input = CORES_PER_TILE + MEMS_PER_TILE;
  for(unsigned int output=0; output<CORES_PER_TILE; output++) {
    globalToCore.requestsIn[0][output](coreRequests[input][output]);
    globalToCore.grantsOut[0][output](coreGrants[input][output]);
  }
}

CreditInput&  NewLocalNetwork::externalCreditIn()       const {return creditsIn[creditInputs-1];}
CreditOutput& NewLocalNetwork::externalCreditOut()      const {return creditsOut[creditOutputs-1];}
ReadyInput&   NewLocalNetwork::externalValidCreditIn()  const {return validCreditIn[creditInputs-1];}
ReadyOutput&  NewLocalNetwork::externalValidCreditOut() const {return validCreditOut[creditOutputs-1];}
ReadyInput&   NewLocalNetwork::externalAckCreditIn()    const {return ackCreditOut[creditOutputs-1];}
ReadyOutput&  NewLocalNetwork::externalAckCreditOut()   const {return ackCreditIn[creditInputs-1];}

NewLocalNetwork::NewLocalNetwork(const sc_module_name& name, ComponentID tile) :
    Network(name, tile, OUTPUT_PORTS_PER_TILE, INPUT_PORTS_PER_TILE, Network::COMPONENT, Dimension(1.0, 0.03), 0, true),
    coreToCore("core_to_core", tile, CORES_PER_TILE, CORES_PER_TILE*CORE_INPUT_PORTS, CORE_INPUT_PORTS, level, size, true),
    coreToMemory("core_to_mem", tile, CORES_PER_TILE, MEMS_PER_TILE, MEMORY_INPUT_PORTS, level, size, false),
    memoryToCore("mem_to_core", tile, MEMS_PER_TILE, CORES_PER_TILE*CORE_INPUT_PORTS, CORE_INPUT_PORTS, level, size, true),
    coreToGlobal("core_to_global", tile, CORES_PER_TILE, 1, 1, level, size, false),
    globalToCore("global_to_core", tile, 1, CORES_PER_TILE, 1, level, size, true),
    c2gCredits("c2g_credits", tile, CORES_PER_TILE, 1, 1, level, size),
    g2cCredits("g2c_credits", tile, 1, CORES_PER_TILE, 1, level, size) {

  creditsIn      = new CreditInput[creditInputs];
  validCreditIn  = new ReadyInput[creditInputs];
  ackCreditIn    = new ReadyOutput[creditInputs];

  creditsOut     = new CreditOutput[creditOutputs];
  validCreditOut = new ReadyOutput[creditOutputs];
  ackCreditOut   = new ReadyInput[creditOutputs];

  createSignals();
  createArbiters();
  createMuxes();
  wireUpSubnetworks();

}

NewLocalNetwork::~NewLocalNetwork() {
  delete[] creditsIn;      delete[] validCreditIn;  delete[] ackCreditIn;
  delete[] creditsOut;     delete[] validCreditOut; delete[] ackCreditOut;

  for(unsigned int i=0; i<CORES_PER_TILE; i++) {
    delete[] dataSig[i];   delete[] validSig[i];    delete[] ackSig[i];
    delete[] selectSig[i]; delete[] requestSig[i];  delete[] grantSig[i];
  }

  delete[] dataSig;        delete[] validSig;       delete[] ackSig;
  delete[] selectSig;      delete[] requestSig;     delete[] grantSig;

  for(unsigned int i=0; i<CORES_PER_TILE+MEMS_PER_TILE+1; i++) {
    delete[] coreRequests[i];   delete[] coreGrants[i];
  }
  for(unsigned int i=0; i<CORES_PER_TILE; i++) {
    delete[] memRequests[i];    delete[] memGrants[i];
    delete[] globalRequests[i]; delete[] globalGrants[i];
  }

  delete[] coreRequests;   delete[] memRequests;    delete[] globalRequests;
  delete[] coreGrants;     delete[] memGrants;      delete[] globalGrants;

  for(unsigned int i=0; i<arbiters.size(); i++)     delete arbiters[i];
  for(unsigned int i=0; i<muxes.size();    i++)     delete muxes[i];
}
