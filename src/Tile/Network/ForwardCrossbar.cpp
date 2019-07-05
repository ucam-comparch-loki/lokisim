/*
 * ForwardCrossbar.cpp
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#include "ForwardCrossbar.h"
#include "../ComputeTile.h"

// Each core has one output port.
// Each accelerator has 3 DMAs, each of which has one port for each memory bank.
ForwardCrossbar::ForwardCrossbar(const sc_module_name name,
                                 const tile_parameters_t& params) :
    Network<Word>(name, params.numCores + params.numAccelerators * 3 * params.numMemories, params.numMemories + 1) {
  // All initialisation done in constructor.

}

ForwardCrossbar::~ForwardCrossbar() {
  // Nothing
}

PortIndex ForwardCrossbar::getDestination(const ChannelID address) const {
  // The first N outputs are memory banks on this tile, and the final output
  // is the core-to-core data router.

  if (parent().isMemory(address.component))
    return parent().memoryIndex(address.component);
  else
    return outputs.size() - 1;
}

ComputeTile& ForwardCrossbar::parent() const {
  return *(static_cast<ComputeTile*>(this->get_parent_object()));
}
