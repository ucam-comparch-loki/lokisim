/*
 * L2LToBankRequests.cpp
 *
 *  Created on: 3 Apr 2019
 *      Author: db434
 */

#include "L2LToBankRequests.h"
#include "../../Utility/Assert.h"

L2LToBankRequests::L2LToBankRequests(const sc_module_name name,
                                     const tile_parameters_t& params) :
    Network<Word>(name, 1, params.numMemories) {

  // The network destinations are always the same, so precompute them.
  for (uint i=0; i<params.numMemories; i++)
    allOutputs.insert(i);

}

L2LToBankRequests::~L2LToBankRequests() {
  // Nothing
}

PortIndex L2LToBankRequests::getDestination(const ChannelID address) const {
  // Single destination method should never be used.
  loki_assert(false);
  return -1;
}

set<PortIndex> L2LToBankRequests::getDestinations(const ChannelID address) const {
  return allOutputs;
}

void L2LToBankRequests::newData(PortIndex input) {
  // Wait for all outputs to be available.
  for (uint i=0; i<outputs.size(); i++) {
    if (!outputs[i]->canWrite()) {
      next_trigger(outputs[i]->canWriteEvent());
      return;
    }
  }

  // If all outputs are available, fall back on the normal behaviour.
  Network<Word>::newData(input);
}
