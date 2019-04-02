/*
 * InstructionReturn.cpp
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#include "InstructionReturn.h"
#include "../../Utility/Assert.h"
#include "../Core/Core.h"

// TODO: Need slightly different arbitration from the default network.
// Default is to have one arbiter per output port. Here, we want one arbiter
// per output core, which may have many ports. DataReturn is the same.

InstructionReturn::InstructionReturn(const sc_module_name name,
                                     const tile_parameters_t& params) :
    Network<Word>(name, params.numMemories, params.numCores * Core::numInstructionChannels),
    outputsPerCore(Core::numInstructionChannels),
    outputCores(params.numCores) {

  // Nothing

}

InstructionReturn::~InstructionReturn() {
  // Nothing
}

PortIndex InstructionReturn::getDestination(const ChannelID address) const {
  uint core = address.component.position;
  uint channel = address.channel;

  // Only instruction buffers are accessible through this network.
  loki_assert(channel < Core::numInstructionChannels);

  return (core * outputsPerCore) + channel;
}
