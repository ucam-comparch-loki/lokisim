/*
 * CoreMulticast.cpp
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "CoreMulticast.h"
#include "../../Network/Topologies/MulticastBus.h"

CoreMulticast::CoreMulticast(const sc_module_name name, ComponentID tile) :
    Network(name, tile, MULTICAST_NETWORK_SIZE, MULTICAST_NETWORK_SIZE*MULTICAST_NETWORK_SIZE, Network::COMPONENT),
    iData(MULTICAST_NETWORK_SIZE, "iData"),
    oData(MULTICAST_NETWORK_SIZE, MULTICAST_NETWORK_SIZE, "oData"),
    busInput(MULTICAST_NETWORK_SIZE, "busInput") {

  state.assign(iData.size(), IDLE);

  for (uint i=0; i<iData.size(); i++) {
    MulticastBus* bus = new MulticastBus(sc_gen_unique_name("bus"), tile, oData.size(), Network::COMPONENT);

    bus->clock(clock);
    bus->iData(busInput[i]);
    for (uint out=0; out<oData.size(); out++)
      bus->oData[out](oData[out][i]);

    buses.push_back(bus);

    // Create a method which puts data onto the bus.
    SPAWN_METHOD(iData[i], CoreMulticast::mainLoop, i, false);
  }

  // TODO: Would be nice for the constructor to receive a list of components to
  // connect so we can check how many connections they have, rather than
  // hard coding this.
  iReady.init(MULTICAST_NETWORK_SIZE);
  for (uint i=0; i<CORES_PER_TILE; i++)
    iReady[i].init(CORE_INPUT_CHANNELS, "iReady");
  for (uint i=CORES_PER_TILE; i<CORES_PER_TILE+ACCELERATORS_PER_TILE; i++)
    iReady[i].init(1, "iReady");

}

CoreMulticast::~CoreMulticast() {
  for (uint i=0; i<buses.size(); i++)
    delete buses[i];
}

void CoreMulticast::mainLoop(PortIndex input) {
  switch (state[input]) {
    case IDLE:
      assert(iData[input].valid());
      state[input] = FLOW_CONTROL;
      next_trigger(sc_core::SC_ZERO_TIME);

      break;

    case FLOW_CONTROL: {
      assert(iData[input].valid());
      ChannelID address = iData[input].read().channelID();
      assert(address.multicast);

      // Check whether all destinations are ready to receive data.
      uint destinations = address.coremask;
      uint buffer = address.channel;
      for (uint output=0; output<iReady.size(); output++) {
        if (((destinations >> output) & 1) && !iReady[output][buffer].read()) {
          next_trigger(iReady[output][buffer].posedge_event());
          break;
        }
      }

      state[input] = SEND;
      next_trigger(sc_core::SC_ZERO_TIME);

      break;
    }

    case SEND:
      busInput[input].write(iData[input].read());
      state[input] = ACKNOWLEDGE;
      next_trigger(busInput[input].ack_event());

      break;

    case ACKNOWLEDGE:
      assert(!busInput[input].valid());
      iData[input].ack();
      state[input] = IDLE;

      // Default trigger: new data.

      break;
  }
}
