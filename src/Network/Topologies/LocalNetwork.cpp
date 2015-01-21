/*
 * NewLocalNetwork.cpp
 *
 *  Created on: 9 Sep 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "LocalNetwork.h"

void LocalNetwork::makeRequest(ComponentID source, ChannelID destination, bool request) {
  if (destination.isMulticast())
    multicastRequest(source, destination, request);
  else
    pointToPointRequest(source, destination, request);
}

void LocalNetwork::multicastRequest(ComponentID source, ChannelID destination, bool request) {
  assert(destination.isMulticast());

  // For multicast addresses, instead of a component ID, we have a bitmask of
  // cores in this tile which should receive the data.
  unsigned int bitmask = destination.getCoreMask();

  // Loop through all bits in the bitmask, and send a request for each bit set.
  for (uint i=0; i<CORES_PER_TILE; i++) {
    if ((bitmask >> i) & 1) {
      ChannelID core(id.getTileColumn(), id.getTileRow(), i, destination.getChannel());
      pointToPointRequest(source, core, request);
    }
  }
}

void LocalNetwork::pointToPointRequest(ComponentID source, ChannelID destination, bool request) {
  assert(!destination.isMulticast());

  // Find out which signal to write the request to.
  ArbiterRequestSignal *requestSignal;

  if (!source.isCore()) {                             // Memory/global to core
    assert(destination.isCore());
    requestSignal = &coreRequests[source.getPosition()][destination.getPosition()];
  }
  else {
    if (destination.isMemory())                       // Core to memory
      requestSignal = &memRequests[source.getPosition()][destination.getPosition()-CORES_PER_TILE];
    else                                              // Core to core
      requestSignal = &coreRequests[source.getPosition()][destination.getPosition()];
  }

  // Send the request.
  if (request)
    requestSignal->write(destination.getChannel());
  else
    requestSignal->write(NO_REQUEST);
}

bool LocalNetwork::requestGranted(ComponentID source, ChannelID destination) const {
  // Multicast requests are treated specially: they are allowed onto the
  // network immediately, and then all arbiters select the data at their own
  // pace.
  if (destination.isMulticast())
    return true;

  ArbiterGrantSignal   *grantSignal;

  if (!source.isCore()) {                             // Memory/global to core
    assert(destination.isCore());
    grantSignal   = &coreGrants[source.getPosition()][destination.getPosition()];
  }
  else {
    if (destination.isMemory())                       // Core to memory
      grantSignal   = &memGrants[source.getPosition()][destination.getPosition()-CORES_PER_TILE];
    else                                              // Core to core
      grantSignal   = &coreGrants[source.getPosition()][destination.getPosition()];
  }

  return grantSignal->read();
}

void LocalNetwork::createSignals() {
  dataSig.init(CORES_PER_TILE, 2); // each core can send to 2 subnetworks

  int numCores = CORES_PER_TILE;
  int sendToCores = CORES_PER_TILE + MEMS_PER_TILE;// + 1;
  coreRequests.init(sendToCores, numCores);
  coreGrants.init(sendToCores, numCores);
  for (int i=0; i<sendToCores; i++)
    for (int j=0; j<numCores; j++)
      coreRequests[i][j].write(NO_REQUEST);

  int numMems = MEMS_PER_TILE;
  int sendToMems = CORES_PER_TILE;
  memRequests.init(sendToMems, numMems);
  memGrants.init(sendToMems, numMems);
  for (int i=0; i<sendToMems; i++)
    for (int j=0; j<numMems; j++)
      memRequests[i][j].write(NO_REQUEST);
}

void LocalNetwork::wireUpSubnetworks() {
  // Each subnetwork contains arbiters, and the outputs of the networks
  // are themselves arbitrated. Use the slow clock because the memory sends
  // its data on the negative edge, and the core may need the time to compute
  // which memory bank it is sending to.
  coreToCore.clock(slowClock);
  memoryToCore.clock(slowClock); coreToMemory.clock(slowClock);

  // Keep track of how many ports in each array have been bound, so that if
  // multiple networks connect to the same array, they can start where the
  // previous network finished.
  int portsBound = 0;

  // Data networks
  for (unsigned int i=0; i<coreToCore.numInputPorts(); i++, portsBound++) {
    // Data from cores can go to all three core-to-X networks.
    coreToCore.iData[i](dataSig[portsBound][CORE]);
    coreToMemory.iData[i](dataSig[portsBound][MEMORY]);
  }
  for (unsigned int i=0; i<memoryToCore.numInputPorts(); i++, portsBound++)
    memoryToCore.iData[i](iData[portsBound]);

  // Cores have the following data inputs:
  //   2 from core-to-core network
  //   2 from memory-to-core network
  portsBound = 0;
  for (unsigned int i=0; i<CORES_PER_TILE; i++) {
    coreToCore.oData[2*i](oData[portsBound++]);
    coreToCore.oData[2*i + 1](oData[portsBound++]);
    memoryToCore.oData[2*i](oData[portsBound++]);
    memoryToCore.oData[2*i + 1](oData[portsBound++]);
  }

  for (unsigned int i=0; i<coreToMemory.numOutputPorts(); i++)
    coreToMemory.oData[i](oData[portsBound++]);

  // Requests/grants for arbitration.
  for (unsigned int input=0; input<CORES_PER_TILE; input++) {
    for (unsigned int output=0; output<CORES_PER_TILE; output++) {
      coreToCore.iRequest[input][output](coreRequests[input][output]);
      coreToCore.oGrant[input][output](coreGrants[input][output]);
    }
    for (unsigned int output=0; output<MEMS_PER_TILE; output++) {
      coreToMemory.iRequest[input][output](memRequests[input][output]);
      coreToMemory.oGrant[input][output](memGrants[input][output]);
    }
  }
  for (unsigned int i=0; i<MEMS_PER_TILE; i++) {
    int input = i + CORES_PER_TILE; // Memories start at offset CORES_PER_TILE
    for (unsigned int output=0; output<CORES_PER_TILE; output++) {
      memoryToCore.iRequest[i][output](coreRequests[input][output]);
      memoryToCore.oGrant[i][output](coreGrants[input][output]);
    }
  }

  // Ready signals
  for (unsigned int core=0; core<CORES_PER_TILE; core++)
    for (unsigned int channel=0; channel<CORE_INPUT_CHANNELS; channel++) {
      coreToCore.iReady[core][channel](iReady[core][channel]);
      memoryToCore.iReady[core][channel](iReady[core][channel]);
    }
  for (unsigned int mem=0; mem<MEMS_PER_TILE; mem++)
    for (unsigned int channel=0; channel<1; channel++)
      coreToMemory.iReady[mem][channel](iReady[mem + CORES_PER_TILE][channel]);
}

// TODO: is it possible to remove this method and directly connect the
// subnetworks to the appropriate input ports? Won't matter when the core has
// separate ports for each network.
void LocalNetwork::newCoreData(int core) {
  const NetworkData& data = iData[core].read();
  const ChannelID& destination = data.channelID();

  if (destination.isCore())
    dataSig[core][CORE].write(data);
  else if (destination.isMemory())
    dataSig[core][MEMORY].write(data);
}

void LocalNetwork::coreDataAck(int core) {
  iData[core].ack();
}

LocalNetwork::LocalNetwork(const sc_module_name& name, ComponentID tile) :
    Network(name, tile, OUTPUT_PORTS_PER_TILE, INPUT_PORTS_PER_TILE, Network::COMPONENT, 0),
    coreToCore("core_to_core", tile, CORES_PER_TILE, CORES_PER_TILE*2, 2, CORE_INPUT_CHANNELS),
    coreToMemory("core_to_mem", tile, CORES_PER_TILE, MEMS_PER_TILE, MEMORY_INPUT_PORTS, level, 1),
    memoryToCore("mem_to_core", tile, MEMS_PER_TILE, CORES_PER_TILE*2, 2, level, CORE_INPUT_CHANNELS) {

  // Ready signals from each component.//, plus one from the router.
  iReady.init(COMPONENTS_PER_TILE);
  for (size_t i=0; i<iReady.length(); i++) {
    if (i<CORES_PER_TILE)           // cores have multiple buffers
      iReady[i].init(CORE_INPUT_CHANNELS);
    else if (i<COMPONENTS_PER_TILE) // memories have one buffer each
      iReady[i].init(1);
  }

  createSignals();
  wireUpSubnetworks();

  for (unsigned int i=0; i<CORES_PER_TILE; i++) {
    // Method to handle data from the core.
    SPAWN_METHOD(iData[i], LocalNetwork::newCoreData, i, false);

    // Method to handle acks to the core. Can't use the macro because we're
    // sensitive to multiple things.
    sc_core::sc_spawn_options options;
    options.spawn_method();
    options.set_sensitivity(&(dataSig[i][CORE].ack_event()));
    options.set_sensitivity(&(dataSig[i][MEMORY].ack_event()));
    options.dont_initialize();

    /* Create the method. */
    sc_spawn(sc_bind(&LocalNetwork::coreDataAck, this, i), 0, &options);
  }

}
