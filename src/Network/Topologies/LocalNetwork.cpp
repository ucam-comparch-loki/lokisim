/*
 * NewLocalNetwork.cpp
 *
 *  Created on: 9 Sep 2011
 *      Author: db434
 */

#include "LocalNetwork.h"
#include "../Multiplexer.h"
#include "../Arbiters/EndArbiter.h"
#include "../../OrGate.h"

// Only cores and the global network can send/receive credits.
const unsigned int LocalNetwork::creditInputs  = CORES_PER_TILE + 1;
const unsigned int LocalNetwork::creditOutputs = CORES_PER_TILE + 1;

// 2 inputs each from core-to-core and memory-to-core networks, plus one
// from the router.
const unsigned int LocalNetwork::muxInputs = CORE_INPUT_PORTS + CORE_INPUT_PORTS + 1;

void LocalNetwork::makeRequest(ComponentID source, ChannelID destination, bool request) {

  // Find out which signal to write the request to.
  RequestSignal *requestSignal;

  if(source.isMemory()) {                             // Memory to core
    assert(destination.isCore());
    requestSignal = &coreRequests[source.getPosition()][destination.getPosition()];
  }
  else if(source.getTile() != id.getTile()) {         // Global to core
    assert(destination.isCore());
    requestSignal = &coreRequests[COMPONENTS_PER_TILE][destination.getPosition()];
  }
  else {
    if(destination.isMemory())                        // Core to memory
      requestSignal = &memRequests[source.getPosition()][destination.getPosition()-CORES_PER_TILE];
    else if(destination.getTile() != id.getTile())    // Core to global
      requestSignal = &globalRequests[source.getPosition()][0];
    else                                              // Core to core
      requestSignal = &coreRequests[source.getPosition()][destination.getPosition()];
  }

  // Send the request.
  if(request)
    requestSignal->write(destination.getChannel());
  else
    requestSignal->write(NO_REQUEST);
}

bool LocalNetwork::requestGranted(ComponentID source, ChannelID destination) const {
  GrantSignal   *grantSignal;

  if(source.isMemory()) {                             // Memory to core
    assert(destination.isCore());
    grantSignal   = &coreGrants[source.getPosition()][destination.getPosition()];
  }
  else if(source.getTile() != id.getTile()) {         // Global to core
    assert(destination.isCore());
    grantSignal   = &coreGrants[COMPONENTS_PER_TILE][destination.getPosition()];
  }
  else {
    if(destination.isMemory())                        // Core to memory
      grantSignal   = &memGrants[source.getPosition()][destination.getPosition()-CORES_PER_TILE];
    else if(destination.getTile() != id.getTile())    // Core to global
      grantSignal   = &globalGrants[source.getPosition()][0];
    else                                              // Core to core
      grantSignal   = &coreGrants[source.getPosition()][destination.getPosition()];
  }

  return grantSignal->read();
}

void LocalNetwork::createArbiters() {
  for(unsigned int i=0; i<CORES_PER_TILE; i++) {
    EndArbiter* arb = new EndArbiter(sc_gen_unique_name("arbiter"),
                                     id,
                                     muxInputs,
                                     CORE_INPUT_PORTS,
                                     true);

    // Is this right? Does the arbiter need to have its clock skewed at all?
    arb->clock(arbiterClock);

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

void LocalNetwork::createMuxes() {
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
    }

    mux->dataOut(dataOut[i]);

    muxes.push_back(mux);
  }
}

void LocalNetwork::createSignals() {
  dataSig    = new DataSignal*[CORES_PER_TILE];
  selectSig  = new SelectSignal*[CORES_PER_TILE];
  requestSig = new RequestSignal*[CORES_PER_TILE];
  grantSig   = new GrantSignal*[CORES_PER_TILE];

  for(unsigned int i=0; i<CORES_PER_TILE; i++) {
    dataSig[i]    = new DataSignal[muxInputs];

    selectSig[i]  = new SelectSignal[CORE_INPUT_PORTS];
    requestSig[i] = new RequestSignal[muxInputs];
    grantSig[i]   = new GrantSignal[muxInputs];

    for(unsigned int j=0; j<muxInputs; j++)
      requestSig[i][j].write(NO_REQUEST);
  }

  int numCores = CORES_PER_TILE;
  int sendToCores = CORES_PER_TILE + MEMS_PER_TILE + 1;
  coreRequests = new RequestSignal*[sendToCores];
  coreGrants   = new GrantSignal*[sendToCores];
  for(int i=0; i<sendToCores; i++) {
    coreRequests[i] = new RequestSignal[numCores];
    coreGrants[i]   = new GrantSignal[numCores];

    for(int j=0; j<numCores; j++)
      coreRequests[i][j].write(NO_REQUEST);
  }

  int numMems = MEMS_PER_TILE;
  int sendToMems = CORES_PER_TILE;
  memRequests = new RequestSignal*[sendToMems];
  memGrants   = new GrantSignal*[sendToMems];
  for(int i=0; i<sendToMems; i++) {
    memRequests[i] = new RequestSignal[numMems];
    memGrants[i]   = new GrantSignal[numMems];

    for(int j=0; j<numMems; j++)
      memRequests[i][j].write(NO_REQUEST);
  }

  int numRouterLinks = 1;
  int sendToRouters = CORES_PER_TILE;
  globalRequests = new RequestSignal*[sendToRouters];
  globalGrants   = new GrantSignal*[sendToRouters];
  for(int i=0; i<sendToRouters; i++) {
    globalRequests[i] = new RequestSignal[numRouterLinks];
    globalGrants[i]   = new GrantSignal[numRouterLinks];

    for(int j=0; j<numRouterLinks; j++)
      globalRequests[i][j].write(NO_REQUEST);
  }
}

void LocalNetwork::wireUpSubnetworks() {
  // Each subnetwork contains arbiters, and the outputs of the networks
  // are themselves arbitrated. Use the slow clock because the memory sends
  // its data on the negative edge, and the core may need the time to compute
  // which memory bank it is sending to.
  coreToCore.clock(slowClock); coreToMemory.clock(slowClock); coreToGlobal.clock(slowClock);
  memoryToCore.clock(slowClock); globalToCore.clock(slowClock);
  c2gCredits.clock(clock); g2cCredits.clock(clock);   // Use fastClock too?

  // Keep track of how many ports in each array have been bound, so that if
  // multiple networks connect to the same array, they can start where the
  // previous network finished.
  int portsBound = 0;

  // Data networks
  for(unsigned int i=0; i<coreToCore.numInputPorts(); i++) {
    // Data from cores can go to all three core-to-X networks. In practice,
    // there might be a demux to reduce switching.
    coreToCore.dataIn[i](dataIn[portsBound]);
    coreToMemory.dataIn[i](dataIn[portsBound]);
    coreToGlobal.dataIn[i](dataIn[portsBound]);

    portsBound++;
  }
  for(unsigned int i=0; i<memoryToCore.numInputPorts(); i++) {
    memoryToCore.dataIn[i](dataIn[portsBound]);
    portsBound++;
  }
  for(unsigned int i=0; i<globalToCore.numInputPorts(); i++) {
    globalToCore.dataIn[i](dataIn[portsBound]);
    portsBound++;
  }

  for(unsigned int i=0; i<coreToCore.numOutputPorts(); i++) {
    int arbiter = i/CORE_INPUT_PORTS;
    int arbiterInput = i%CORE_INPUT_PORTS;
    coreToCore.dataOut[i](dataSig[arbiter][arbiterInput]);

    int arbiterOutput = i % CORE_INPUT_PORTS;
    coreToCore.requestsOut[arbiter][arbiterOutput](requestSig[arbiter][arbiterInput]);
    coreToCore.grantsIn[arbiter][arbiterOutput](grantSig[arbiter][arbiterInput]);
  }
  for(unsigned int i=0; i<memoryToCore.numOutputPorts(); i++) {
    int arbiter = i/CORE_INPUT_PORTS;
    int arbiterInput = (i%CORE_INPUT_PORTS) + CORE_INPUT_PORTS;
    memoryToCore.dataOut[i](dataSig[arbiter][arbiterInput]);

    int arbiterOutput = i % CORE_INPUT_PORTS;
    memoryToCore.requestsOut[arbiter][arbiterOutput](requestSig[arbiter][arbiterInput]);
    memoryToCore.grantsIn[arbiter][arbiterOutput](grantSig[arbiter][arbiterInput]);
  }
  for(unsigned int i=0; i<globalToCore.numOutputPorts(); i++) {
    int arbiter = i;  // Only one output per core
    int arbiterInput = CORE_INPUT_PORTS + CORE_INPUT_PORTS;
    globalToCore.dataOut[i](dataSig[arbiter][arbiterInput]);

    int arbiterOutput = 0;
    globalToCore.requestsOut[arbiter][arbiterOutput](requestSig[arbiter][arbiterInput]);
    globalToCore.grantsIn[arbiter][arbiterOutput](grantSig[arbiter][arbiterInput]);
  }
  for(unsigned int i=0; i<coreToMemory.numOutputPorts(); i++) {
    // Some ports will have already been connected.
    int port = i + (CORES_PER_TILE*CORE_INPUT_PORTS);
    coreToMemory.dataOut[i](dataOut[port]);
  }
  for(unsigned int i=0; i<coreToGlobal.numOutputPorts(); i++) {
    // Some ports will have already been connected.
    int port = i + (CORES_PER_TILE*CORE_INPUT_PORTS)
                 + (MEMS_PER_TILE*MEMORY_INPUT_PORTS);
    coreToGlobal.dataOut[i](dataOut[port]);
  }

  // Credit networks
  portsBound = 0;
  for(unsigned int i=0; i<c2gCredits.numInputPorts(); i++) {
    c2gCredits.dataIn[i](creditsIn[portsBound]);
    portsBound++;
  }
  for(unsigned int i=0; i<g2cCredits.numInputPorts(); i++) {
    g2cCredits.dataIn[i](creditsIn[portsBound]);
    portsBound++;
  }

  portsBound = 0;
  for(unsigned int i=0; i<g2cCredits.numOutputPorts(); i++) {
    g2cCredits.dataOut[i](creditsOut[portsBound]);
    portsBound++;
  }
  for(unsigned int i=0; i<c2gCredits.numOutputPorts(); i++) {
    c2gCredits.dataOut[i](creditsOut[portsBound]);
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

  // Ready signals
  for(unsigned int i=0; i<CORES_PER_TILE; i++)
    for(unsigned int j=0; j<CORE_INPUT_CHANNELS; j++)
      arbiters[i]->readyIn[j](readyIn[i*CORE_INPUT_CHANNELS + j]);
  for(unsigned int i=0; i<MEMS_PER_TILE; i++)
    coreToMemory.readyIn[i](readyIn[i + CORES_PER_TILE*CORE_INPUT_CHANNELS]);
  coreToGlobal.readyIn[0](readyIn[INPUT_CHANNELS_PER_TILE]);
}

ReadyInput&   LocalNetwork::externalReadyInput() const {return readyIn[INPUT_CHANNELS_PER_TILE];}
CreditInput&  LocalNetwork::externalCreditIn()   const {return creditsIn[creditInputs-1];}
CreditOutput& LocalNetwork::externalCreditOut()  const {return creditsOut[creditOutputs-1];}

LocalNetwork::LocalNetwork(const sc_module_name& name, ComponentID tile) :
    NewNetwork(name, tile, OUTPUT_PORTS_PER_TILE, INPUT_PORTS_PER_TILE, NewNetwork::COMPONENT, Dimension(1.0, 0.03), 0, true),
    arbiterClock("test", sc_core::sc_time(1.0, sc_core::SC_NS), 0.8),
    coreToCore("core_to_core", tile, CORES_PER_TILE, CORES_PER_TILE*CORE_INPUT_PORTS, CORE_INPUT_PORTS, level, size, true),
    coreToMemory("core_to_mem", tile, CORES_PER_TILE, MEMS_PER_TILE, MEMORY_INPUT_PORTS, level, size, false),
    memoryToCore("mem_to_core", tile, MEMS_PER_TILE, CORES_PER_TILE*CORE_INPUT_PORTS, CORE_INPUT_PORTS, level, size, true),
    coreToGlobal("core_to_global", tile, CORES_PER_TILE, 1, 1, level, size, false),
    globalToCore("global_to_core", tile, 1, CORES_PER_TILE, 1, level, size, true),
    c2gCredits("c2g_credits", tile, CORES_PER_TILE, 1, 1, (Network::HierarchyLevel)level, size),
    g2cCredits("g2c_credits", tile, 1, CORES_PER_TILE, 1, (Network::HierarchyLevel)level, size) {

  creditsIn      = new CreditInput[creditInputs];
  creditsOut     = new CreditOutput[creditOutputs];

  // Ready signals from each component, plus one from the router.
  readyIn        = new ReadyInput[INPUT_CHANNELS_PER_TILE + 1];

  createSignals();
  createArbiters();
  createMuxes();
  wireUpSubnetworks();

}

LocalNetwork::~LocalNetwork() {
  delete[] creditsIn;
  delete[] creditsOut;

  for(unsigned int i=0; i<CORES_PER_TILE; i++) {
    delete[] dataSig[i];
    delete[] selectSig[i]; delete[] requestSig[i];  delete[] grantSig[i];
  }

  delete[] dataSig;
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
