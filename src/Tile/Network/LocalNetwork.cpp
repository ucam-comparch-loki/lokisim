/*
 * NewLocalNetwork.cpp
 *
 *  Created on: 9 Sep 2011
 *      Author: db434
 */
#include <systemc>


#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "LocalNetwork.h"
#include "../../Utility/Assert.h"

using sc_core::sc_module_name;

void LocalNetwork::makeRequest(ComponentID source, ChannelID destination, bool request) {
  if (destination.multicast)
    multicastRequest(source, destination, request);
  else
    pointToPointRequest(source, destination, request);
}

void LocalNetwork::multicastRequest(ComponentID source, ChannelID destination, bool request) {
  assert(destination.multicast);

  // For multicast addresses, instead of a component ID, we have a bitmask of
  // cores in this tile which should receive the data.
  unsigned int bitmask = destination.coremask;

  // Loop through all bits in the bitmask, and send a request for each bit set.
  for (uint i=0; i<CORES_PER_TILE; i++) {
    if ((bitmask >> i) & 1) {
      ChannelID core(id.tile.x, id.tile.y, i, destination.channel);
      pointToPointRequest(source, core, request);
    }
  }
}

void LocalNetwork::pointToPointRequest(ComponentID source, ChannelID destination, bool request) {
  loki_assert(!destination.multicast);

  // Find out which signal to write the request to.
  ArbiterRequestSignal *requestSignal;

  if (!source.isCore()) {                             // Memory/global to core
    loki_assert(destination.isCore());
    requestSignal = &coreRequests[source.position][destination.component.position];
  }
  else {
    if (destination.isMemory())                       // Core to memory
      requestSignal = &memRequests[source.position][destination.component.position-CORES_PER_TILE];
    else                                              // Core to core
      requestSignal = &coreRequests[source.position][destination.component.position];
  }

  // Send the request.
  if (request)
    requestSignal->write(destination.channel);
  else
    requestSignal->write(NO_REQUEST);
}

bool LocalNetwork::requestGranted(ComponentID source, ChannelID destination) const {
  // Multicast requests are treated specially: they are allowed onto the
  // network immediately, and then all arbiters select the data at their own
  // pace.
  if (destination.multicast)
    return true;

  ArbiterGrantSignal   *grantSignal;

  if (!source.isCore()) {                             // Memory/global to core
    loki_assert(destination.isCore());
    grantSignal   = &coreGrants[source.position][destination.component.position];
  }
  else {
    if (destination.isMemory())                       // Core to memory
      grantSignal   = &memGrants[source.position][destination.component.position-CORES_PER_TILE];
    else                                              // Core to core
      grantSignal   = &coreGrants[source.position][destination.component.position];
  }

  return grantSignal->read();
}

void LocalNetwork::createSignals() {
  dataSig.init(2, CORES_PER_TILE); // each core can send to 2 subnetworks

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

  // Data networks
  coreToCore.iData(dataSig[CORE]);
  coreToMemory.iData(dataSig[MEMORY]);

  // For each of the core's input buffers, there is a separate port from other
  // cores and from memory banks.
  for (uint core=0; core<CORES_PER_TILE; core++) {
    for (uint buffer=0; buffer<CORE_INPUT_CHANNELS; buffer++) {
      coreToCore.oData[CORE_INPUT_CHANNELS*core + buffer](oData[core][2*buffer]);
      memoryToCore.oData[CORE_INPUT_CHANNELS*core + buffer](oData[core][2*buffer + 1]);
    }
  }

  for (uint mem=0; mem<MEMS_PER_TILE; mem++) {
    uint position = mem + CORES_PER_TILE;
    memoryToCore.iData[mem](iData[position][0]);
    coreToMemory.oData[mem](oData[position][0]);
  }

  // Requests/grants for arbitration.
  coreToMemory.iRequest(memRequests);
  coreToMemory.oGrant(memGrants);
  for (uint input=0; input<CORES_PER_TILE; input++) {
    coreToCore.iRequest[input](coreRequests[input]);
    coreToCore.oGrant[input](coreGrants[input]);
  }
  for (uint i=0; i<MEMS_PER_TILE; i++) {
    uint input = i + CORES_PER_TILE; // Memories start at offset CORES_PER_TILE
    memoryToCore.iRequest[i](coreRequests[input]);
    memoryToCore.oGrant[i](coreGrants[input]);
  }

  // Ready signals.
  for (uint core=0; core<CORES_PER_TILE; core++) {
    coreToCore.iReady[core](iReady[core]);
    memoryToCore.iReady[core](iReady[core]);
  }
  for (uint mem=0; mem<MEMS_PER_TILE; mem++)
    coreToMemory.iReady[mem](iReady[mem + CORES_PER_TILE]);
}

// TODO: is it possible to remove this method and directly connect the
// subnetworks to the appropriate input ports? Won't matter when the core has
// separate ports for each network.
void LocalNetwork::newCoreData(int core) {
  const NetworkData& data = iData[core][0].read();
  const ChannelID& destination = data.channelID();

  if (destination.isCore())
    dataSig[CORE][core].write(data);
  else if (destination.isMemory())
    dataSig[MEMORY][core].write(data);
}

void LocalNetwork::coreDataAck(int core) {
  iData[core][0].ack();
}

LocalNetwork::LocalNetwork(const sc_module_name& name, ComponentID tile) :
    Network(name, tile, OUTPUT_PORTS_PER_TILE, INPUT_PORTS_PER_TILE, Network::COMPONENT, 0),
    coreToCore("c2c", tile),
    coreToMemory("fwdxbar", tile),
    memoryToCore("dxbar", tile) {

  // Signals to/from each component.
  iData.init(COMPONENTS_PER_TILE);
  oData.init(COMPONENTS_PER_TILE);
  iReady.init(COMPONENTS_PER_TILE);

  for (size_t i=0; i<COMPONENTS_PER_TILE; i++) {
    if (i<CORES_PER_TILE) {
      iData[i].init(CORE_OUTPUT_PORTS);
      oData[i].init(CORE_INPUT_PORTS);
      iReady[i].init(CORE_INPUT_CHANNELS);
    }
    else {
      iData[i].init(1);
      oData[i].init(1);
      iReady[i].init(1);
    }
  }

  createSignals();
  wireUpSubnetworks();

  for (uint core=0; core<CORES_PER_TILE; core++) {
    // Method to handle data from the core.
    SPAWN_METHOD(iData[core][0], LocalNetwork::newCoreData, core, false);

    // Method to handle acks to the core. Can't use the macro because we're
    // sensitive to multiple things.
    sc_core::sc_spawn_options options;
    options.spawn_method();
    options.set_sensitivity(&(dataSig[CORE][core].ack_event()));
    options.set_sensitivity(&(dataSig[MEMORY][core].ack_event()));
    options.dont_initialize();

    /* Create the method. */
    sc_spawn(sc_bind(&LocalNetwork::coreDataAck, this, core), 0, &options);
  }

}
