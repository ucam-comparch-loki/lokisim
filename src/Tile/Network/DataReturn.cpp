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
        params.numCores * params.core.numInputChannels),
    outputsPerCore(params.core.numInputChannels),
    outputCores(params.numCores) {

  // Nothing

}

DataReturn::~DataReturn() {
  // Nothing
}

PortIndex DataReturn::getDestination(const ChannelID address) const {
  uint core = address.component.position;
  uint channel = address.channel;

  return (core * outputsPerCore) + channel;
}
