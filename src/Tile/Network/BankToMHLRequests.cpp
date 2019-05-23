/*
 * BankToMHLRequests.cpp
 *
 *  Created on: 3 Apr 2019
 *      Author: db434
 */

#include "BankToMHLRequests.h"

BankToMHLRequests::BankToMHLRequests(const sc_module_name name,
                                     const tile_parameters_t& params) :
    Network<Word>(name, params.numMemories, 1) {

  // Nothing

}

BankToMHLRequests::~BankToMHLRequests() {
  // Nothing
}

PortIndex BankToMHLRequests::getDestination(const ChannelID address) const {
  // There is only one output - the miss handling logic.
  return 0;
}
