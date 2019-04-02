/*
 * DataReturn.cpp
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#include "DataReturn.h"
#include "../../Utility/Assert.h"
#include "../Core/Core.h"

// TODO: Need slightly different arbitration from the default network.
// Default is to have one arbiter per output port. Here, we want one arbiter
// per output core, which may have many ports. InstructionReturn is the same.

DataReturn::DataReturn(const sc_module_name name,
                       const tile_parameters_t& params) :
    Network<Word>(name, params.numMemories + 1,
        params.numCores * (params.core.numInputChannels-Core::numInstructionChannels)),
    outputsPerCore(params.core.numInputChannels-Core::numInstructionChannels),
    outputCores(params.numCores) {

  // Nothing

}

DataReturn::~DataReturn() {
  // Nothing
}

PortIndex DataReturn::getDestination(const ChannelID address) const {
  uint core = address.component.position;
  uint channel = address.channel;

  // Only data buffers are accessible through this network.
  loki_assert(channel >= Core::numInstructionChannels);
  channel -= Core::numInstructionChannels;

  return (core * outputsPerCore) + channel;
}
