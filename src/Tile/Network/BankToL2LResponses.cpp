/*
 * BankToL2LResponses.cpp
 *
 *  Created on: 3 Apr 2019
 *      Author: db434
 */

#include "BankToL2LResponses.h"

BankToL2LResponses::BankToL2LResponses(const sc_module_name name,
                                     const tile_parameters_t& params) :
    Network<Word>(name, params.numMemories, 1) {

  // Nothing

}

BankToL2LResponses::~BankToL2LResponses() {
  // Nothing
}

PortIndex BankToL2LResponses::getDestination(const ChannelID address) const {
  // There is only one output - the L2 logic.
  return 0;
}
