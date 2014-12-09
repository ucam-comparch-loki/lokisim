/*
 * ResponseNetwork.cpp
 *
 *  Created on: 15 Jul 2014
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "ResponseNetwork.h"
#include "../../TileComponents/Memory/MemoryBank.h"
#include "../Router.h"

void ResponseNetwork::collectOffchipResponses(ResponseSignal* signal) {
  // Multiple copies of this function may be called in the same cycle. This is
  // fine as the buffer they write to is only for modelling purposes, and not
  // part of the implementation.

  assert(signal->valid());
  toMainMemory.write(signal->read());
  signal->ack();
}

void ResponseNetwork::responsesToMemory() {
  // TODO: wormhole routing.

  if (!iMainMemoryReady.read())
    next_trigger(iMainMemoryReady.posedge_event());
  else if (!clock.posedge())
    next_trigger(clock.posedge_event());
  else if (toMainMemory.empty())
    next_trigger(toMainMemory.writeEvent());
  else {
    oMainMemoryResponse.write(toMainMemory.read());
    next_trigger(oMainMemoryResponse.ack_event());
  }
}

void ResponseNetwork::responsesToNetwork() {
  // Assume the memory is at the bottom left of the chip.
  ResponseSignal& signal = *(globalNetwork.edgeDataInputs()[Router::SOUTH][0]);

  if (iMainMemoryResponse.valid()) {
    assert(!signal.valid());
    signal.write(iMainMemoryResponse.read());
    iMainMemoryResponse.ack();
    next_trigger(signal.ack_event());
  }
  else {
    next_trigger(iMainMemoryResponse.default_event());
  }
}

ResponseNetwork::ResponseNetwork(const sc_module_name &name) :
    NetworkHierarchy2(name, MEMS_PER_TILE, 1, 1),
    toMainMemory(16, string(this->name()) + ".buffer") {

  SC_METHOD(responsesToNetwork);
  SC_METHOD(responsesToMemory);

  for (uint direction = 0; direction < 4; direction++) {
    for (uint tile = 0; tile < globalNetwork.edgeDataOutputs()[direction].size(); tile++) {
      ResponseSignal* signal = globalNetwork.edgeDataOutputs()[direction][tile];
      SPAWN_METHOD(*signal, ResponseNetwork::collectOffchipResponses, signal, false);
    }
  }

}

ResponseNetwork::~ResponseNetwork() {
  // Nothing
}

