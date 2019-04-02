/*
 * ForwardCrossbar.cpp
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#include "ForwardCrossbar.h"

ForwardCrossbar::ForwardCrossbar(const sc_module_name name,
                                 const tile_parameters_t& params) :
    Network<Word>(name, params.numCores, params.numMemories + 1){
  // All initialisation done in constructor.

}

ForwardCrossbar::~ForwardCrossbar() {
  // Nothing
}

PortIndex ForwardCrossbar::getDestination(const ChannelID address) const {
  // The first N outputs are memory banks on this tile, and the final output
  // is the core-to-core data router.

  // A bit hacky - the metadata isn't available here.
  bool isCore = address.component.position < inputs.size();

  if (isCore)
    return outputs.size() - 1;
  else
    // The memories' network addresses start after the cores.
    return address.component.position - inputs.size();
}
