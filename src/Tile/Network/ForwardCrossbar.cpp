/*
 * ForwardCrossbar.cpp
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#include "ForwardCrossbar.h"

ForwardCrossbar::ForwardCrossbar(const sc_module_name name,
                                 const tile_parameters_t& params) :
    Network2<Word>(name, params.numCores, params.numMemories){
  // All initialisation done in constructor.

}

ForwardCrossbar::~ForwardCrossbar() {
  // Nothing
}

PortIndex ForwardCrossbar::getDestination(const ChannelID address) const {
  // The memories' network addresses start after the cores.
  return address.component.position - inputs.size();
}
