/*
 * RequestNetwork.cpp
 *
 *  Created on: 15 Jul 2014
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "RequestNetwork.h"
#include "../../TileComponents/Memory/MemoryBank.h"
#include "../Router.h"

void RequestNetwork::collectOffchipRequests(RequestSignal* signal) {
  // Multiple copies of this function may be called in the same cycle. This is
  // fine as the buffer they write to is only for modelling purposes, and not
  // part of the implementation.

  assert(signal->valid());
  toMainMemory.write(signal->read());
  signal->ack();
}

void RequestNetwork::requestsToMemory() {
  // TODO: wormhole routing.

  if (!iMainMemoryReady.read())
    next_trigger(iMainMemoryReady.posedge_event());
  else if (!clock.posedge())
    next_trigger(clock.posedge_event());
  else if (toMainMemory.empty())
    next_trigger(toMainMemory.writeEvent());
  else {
    oMainMemoryRequest.write(toMainMemory.read());
    next_trigger(oMainMemoryRequest.ack_event());
  }
}

void RequestNetwork::requestsToNetwork() {
  // Assume the memory is at the bottom left of the chip.
  RequestSignal& signal = *(globalNetwork.edgeDataInputs()[Router::SOUTH][0]);

  if (iMainMemoryRequest.valid()) {
    assert(!signal.valid());
    signal.write(iMainMemoryRequest.read());
    iMainMemoryRequest.ack();
    next_trigger(signal.ack_event());
  }
  else {
    next_trigger(iMainMemoryRequest.default_event());
  }
}

RequestNetwork::RequestNetwork(const sc_module_name &name) :
    NetworkHierarchy2(name, 1, 1, 1),
    toMainMemory(16, string(this->name()) + ".buffer") {

  SC_METHOD(requestsToMemory);

  for (uint direction = 0; direction < 4; direction++) {
    for (uint tile = 0; tile < globalNetwork.edgeDataOutputs()[direction].size(); tile++) {
      RequestSignal* signal = globalNetwork.edgeDataOutputs()[direction][tile];
      SPAWN_METHOD(*signal, RequestNetwork::collectOffchipRequests, signal, false);
    }
  }

}

RequestNetwork::~RequestNetwork() {
  // Nothing
}

