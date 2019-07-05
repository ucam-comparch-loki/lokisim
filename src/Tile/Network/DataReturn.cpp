/*
 * DataReturn.cpp
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#include "DataReturn.h"
#include "../../Utility/Assert.h"
#include "../ComputeTile.h"
#include "../Core/Core.h"

// TODO: Need slightly different arbitration from the default network.
// Default is to have one arbiter per output port. Here, we want one arbiter
// per output core, which may have many ports. InstructionReturn is the same.

// Each core has a number of input port.
// Each accelerator has 3 DMAs, each of which has one port for each memory bank.
DataReturn::DataReturn(const sc_module_name name,
                       const tile_parameters_t& params) :
    Network<Word>(name, params.numMemories + 1,
        params.numCores * params.core.numInputChannels + params.numAccelerators * 3 * params.numMemories),
    outputsPerCore(params.core.numInputChannels),
    outputCores(params.numCores),
    outputsPerAccelerator(params.numMemories) {

  // Nothing

}

DataReturn::~DataReturn() {
  // Nothing
}

PortIndex DataReturn::getDestination(const ChannelID address) const {
  // Only cores and accelerators are connected to the output of this network.
  // parent().isX() might not work because memories have been excluded from
  // the address space.

  if (parent().isCore(address.component)) {
    uint core = parent().coreIndex(address.component);
    uint channel = address.channel;

    return (core * outputsPerCore) + channel;
  }
  else {
    // A bit hacky - assuming that accelerator IDs come after cores and
    // memories.
    uint acc = address.component.position - parent().numCores();
    uint channel = address.channel;

    return (parent().numCores() * outputsPerCore)
         + (acc * outputsPerAccelerator) + channel;
  }
}

ComputeTile& DataReturn::parent() const {
  return *(static_cast<ComputeTile*>(this->get_parent_object()));
}
