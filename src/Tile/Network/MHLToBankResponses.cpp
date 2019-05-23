/*
 * MHLToBankResponses.cpp
 *
 *  Created on: 3 Apr 2019
 *      Author: db434
 */

#include "MHLToBankResponses.h"

MHLToBankResponses::MHLToBankResponses(const sc_module_name name,
                                       const tile_parameters_t& params) :
    Network<Word>(name, 1, params.numMemories) {

  // Nothing

}

MHLToBankResponses::~MHLToBankResponses() {
  // Nothing
}

PortIndex MHLToBankResponses::getDestination(const ChannelID address) const {
  return address.component.position;
}
