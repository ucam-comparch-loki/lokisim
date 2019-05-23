/*
 * CreditReturn.cpp
 *
 *  Created on: 29 Mar 2019
 *      Author: db434
 */

#include "CreditReturn.h"

CreditReturn::CreditReturn(const sc_module_name name,
                           const tile_parameters_t& params) :
    Network<Word>(name, 1, params.numCores) {

  // Nothing

}

PortIndex CreditReturn::getDestination(const ChannelID address) const {
  return address.component.position;
}

