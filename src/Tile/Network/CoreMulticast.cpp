/*
 * CoreMulticast.cpp
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "CoreMulticast.h"
#include "../../Network/Topologies/MulticastBus.h"

CoreMulticast::CoreMulticast(const sc_module_name name, ComponentID tile)
    : Network(name, tile, CORES_PER_TILE, CORES_PER_TILE*CORES_PER_TILE, Network::COMPONENT) {

  iData.init(CORES_PER_TILE);
  oData.init(CORES_PER_TILE, CORES_PER_TILE);
  iReady.init(CORES_PER_TILE, CORE_INPUT_CHANNELS);

  busInput.init(iData.length());

  state.assign(iData.length(), IDLE);

  for (uint i=0; i<iData.length(); i++) {
    MulticastBus* bus = new MulticastBus(sc_gen_unique_name("bus"), tile, oData.length(), Network::COMPONENT);

    bus->clock(clock);
    bus->iData(busInput[i]);
    for (uint out=0; out<oData.length(); out++)
      bus->oData[out](oData[out][i]);

    buses.push_back(bus);

    // Create a method which puts data onto the bus.
    SPAWN_METHOD(iData[i], CoreMulticast::mainLoop, i, false);
  }

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
      for (uint output=0; output<iReady.length(); output++) {
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