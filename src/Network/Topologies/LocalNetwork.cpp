/*
 * NewLocalNetwork.cpp
 *
 *  Created on: 9 Sep 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "LocalNetwork.h"

// Only cores and the global network can send/receive credits.
const unsigned int LocalNetwork::creditInputs  = CORES_PER_TILE + 1;
const unsigned int LocalNetwork::creditOutputs = CORES_PER_TILE + 1;

void LocalNetwork::makeRequest(ComponentID source, ChannelID destination, bool request) {
  if(destination.isMulticast())
    multicastRequest(source, destination, request);
  else
    pointToPointRequest(source, destination, request);
}

void LocalNetwork::multicastRequest(ComponentID source, ChannelID destination, bool request) {
  assert(destination.isMulticast());

  // For multicast addresses, instead of a component ID, we have a bitmask of
  // cores in this tile which should receive the data.
  unsigned int bitmask = destination.getPosition();

  // Loop through all bits in the bitmask, and send a request for each bit set.
  for(int i=0; i<8; i++) {
    if((bitmask >> i) & 1) {
      ChannelID core(destination.getTile(), i, destination.getChannel());
      pointToPointRequest(source, core, request);
    }
  }
}

void LocalNetwork::pointToPointRequest(ComponentID source, ChannelID destination, bool request) {
  assert(!destination.isMulticast());

  // Find out which signal to write the request to.
  RequestSignal *requestSignal;

  if(!source.isCore()) {                              // Memory/global to core
    assert(destination.isCore());
    requestSignal = &coreRequests[source.getPosition()][destination.getPosition()];
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
  // Multicast requests are treated specially: they are allowed onto the
  // network immediately, and then all arbiters select the data at their own
  // pace.
  if(destination.isMulticast())
    return true;

  GrantSignal   *grantSignal;

  if(!source.isCore()) {                              // Memory/global to core
    assert(destination.isCore());
    grantSignal   = &coreGrants[source.getPosition()][destination.getPosition()];
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

void LocalNetwork::createSignals() {
  dataSig.init(CORES_PER_TILE, 3); // each core can send to 3 subnetworks

  // Ready signals from each component, plus one from the router.
  readyIn.init(COMPONENTS_PER_TILE + 1);
  for(unsigned int i=0; i<readyIn.length(); i++) {
    if(i<CORES_PER_TILE)            // cores have multiple buffers
      readyIn[i].init(CORE_INPUT_CHANNELS);
    else if(i<COMPONENTS_PER_TILE)  // memories have one buffer each
      readyIn[i].init(1);
    else                            // router has one buffer
      readyIn[i].init(1);
  }

  int numCores = CORES_PER_TILE;
  int sendToCores = CORES_PER_TILE + MEMS_PER_TILE + 1;
  coreRequests.init(sendToCores, numCores);
  coreGrants.init(sendToCores, numCores);
  for(int i=0; i<sendToCores; i++)
    for(int j=0; j<numCores; j++)
      coreRequests[i][j].write(NO_REQUEST);

  int numMems = MEMS_PER_TILE;
  int sendToMems = CORES_PER_TILE;
  memRequests.init(sendToMems, numMems);
  memGrants.init(sendToMems, numMems);
  for(int i=0; i<sendToMems; i++)
    for(int j=0; j<numMems; j++)
      memRequests[i][j].write(NO_REQUEST);

  int numRouterLinks = 1;
  int sendToRouters = CORES_PER_TILE;
  globalRequests.init(sendToRouters, numRouterLinks);
  globalGrants.init(sendToRouters, numRouterLinks);
  for(int i=0; i<sendToRouters; i++)
    for(int j=0; j<numRouterLinks; j++)
      globalRequests[i][j].write(NO_REQUEST);
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
  for(unsigned int i=0; i<coreToCore.numInputPorts(); i++, portsBound++) {
    // Data from cores can go to all three core-to-X networks.
    coreToCore.dataIn[i](dataSig[portsBound][CORE]);
    coreToMemory.dataIn[i](dataSig[portsBound][MEMORY]);
    coreToGlobal.dataIn[i](dataSig[portsBound][GLOBAL_NETWORK]);
  }
  for(unsigned int i=0; i<memoryToCore.numInputPorts(); i++, portsBound++)
    memoryToCore.dataIn[i](dataIn[portsBound]);
  for(unsigned int i=0; i<globalToCore.numInputPorts(); i++, portsBound++)
    globalToCore.dataIn[i](dataIn[portsBound]);

  // Cores have the following data inputs:
  //   2 from core-to-core network
  //   2 from memory-to-core network
  //   1 from global network
  portsBound = 0;
  for(unsigned int i=0; i<CORES_PER_TILE; i++) {
    coreToCore.dataOut[2*i](dataOut[portsBound++]);
    coreToCore.dataOut[2*i + 1](dataOut[portsBound++]);
    memoryToCore.dataOut[2*i](dataOut[portsBound++]);
    memoryToCore.dataOut[2*i + 1](dataOut[portsBound++]);
    globalToCore.dataOut[i](dataOut[portsBound++]);
  }

  for(unsigned int i=0; i<coreToMemory.numOutputPorts(); i++)
    coreToMemory.dataOut[i](dataOut[portsBound++]);
  for(unsigned int i=0; i<coreToGlobal.numOutputPorts(); i++)
    coreToGlobal.dataOut[i](dataOut[portsBound++]);

  // Credit networks
  portsBound = 0;
  for(unsigned int i=0; i<c2gCredits.numInputPorts(); i++, portsBound++)
    c2gCredits.dataIn[i](creditsIn[portsBound]);
  for(unsigned int i=0; i<g2cCredits.numInputPorts(); i++, portsBound++)
    g2cCredits.dataIn[i](creditsIn[portsBound]);

  portsBound = 0;
  for(unsigned int i=0; i<g2cCredits.numOutputPorts(); i++, portsBound++)
    g2cCredits.dataOut[i](creditsOut[portsBound]);
  for(unsigned int i=0; i<c2gCredits.numOutputPorts(); i++, portsBound++)
    c2gCredits.dataOut[i](creditsOut[portsBound]);

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
  for(unsigned int core=0; core<CORES_PER_TILE; core++)
    for(unsigned int channel=0; channel<CORE_INPUT_CHANNELS; channel++) {
      coreToCore.readyIn[core][channel](readyIn[core][channel]);
      memoryToCore.readyIn[core][channel](readyIn[core][channel]);
      globalToCore.readyIn[core][channel](readyIn[core][channel]);
    }
  for(unsigned int mem=0; mem<MEMS_PER_TILE; mem++)
    for(unsigned int channel=0; channel<1; channel++)
      coreToMemory.readyIn[mem][channel](readyIn[mem + CORES_PER_TILE][channel]);
  coreToGlobal.readyIn[0][0](readyIn[COMPONENTS_PER_TILE][0]);
}

void LocalNetwork::newCoreData(int core) {
  const DataType& data = dataIn[core].read();
  const ChannelID& destination = data.channelID();

  if (destination.getTile() != id.getTile())
    dataSig[core][GLOBAL_NETWORK].write(data);
  else if (destination.isCore())
    dataSig[core][CORE].write(data);
  else if (destination.isMemory())
    dataSig[core][MEMORY].write(data);
}

void LocalNetwork::coreDataAck(int core) {
  dataIn[core].ack();
}

ReadyInput&   LocalNetwork::externalReadyInput() const {return readyIn[COMPONENTS_PER_TILE][0];}
CreditInput&  LocalNetwork::externalCreditIn()   const {return creditsIn[creditInputs-1];}
CreditOutput& LocalNetwork::externalCreditOut()  const {return creditsOut[creditOutputs-1];}

LocalNetwork::LocalNetwork(const sc_module_name& name, ComponentID tile) :
    NewNetwork(name, tile, OUTPUT_PORTS_PER_TILE, INPUT_PORTS_PER_TILE, NewNetwork::COMPONENT, Dimension(1.0, 0.03), 0, true),
    coreToCore("core_to_core", tile, CORES_PER_TILE, CORES_PER_TILE*2, 2, level, size, CORE_INPUT_CHANNELS),
    coreToMemory("core_to_mem", tile, CORES_PER_TILE, MEMS_PER_TILE, MEMORY_INPUT_PORTS, level, size, 1),
    memoryToCore("mem_to_core", tile, MEMS_PER_TILE, CORES_PER_TILE*2, 2, level, size, CORE_INPUT_CHANNELS),
    coreToGlobal("core_to_global", tile, CORES_PER_TILE, 1, 1, level, size, 1),
    globalToCore("global_to_core", tile, 1, CORES_PER_TILE, 1, level, size, CORE_INPUT_CHANNELS),
    c2gCredits("c2g_credits", tile, CORES_PER_TILE, 1, 1, (Network::HierarchyLevel)level, size),
    g2cCredits("g2c_credits", tile, 1, CORES_PER_TILE, 1, (Network::HierarchyLevel)level, size) {

  creditsIn.init(creditInputs);
  creditsOut.init(creditOutputs);

  createSignals();
  wireUpSubnetworks();

  for (unsigned int i=0; i<CORES_PER_TILE; i++) {
    // Method to handle data from the core.
    SPAWN_METHOD(dataIn[i], LocalNetwork::newCoreData, i, false);

    // Method to handle acks to the core. Can't use the macro because we're
    // sensitive to multiple things.
    sc_core::sc_spawn_options options;
    options.spawn_method();
    options.set_sensitivity(&(dataSig[i][CORE].ack_event()));
    options.set_sensitivity(&(dataSig[i][MEMORY].ack_event()));
    options.set_sensitivity(&(dataSig[i][GLOBAL_NETWORK].ack_event()));
    options.dont_initialize();

    /* Create the method. */
    sc_spawn(sc_bind(&LocalNetwork::coreDataAck, this, i), 0, &options);
  }

}
