/*
 * MissHandlingLogic.cpp
 *
 *  Created on: 8 Oct 2014
 *      Author: db434
 */

#include "MissHandlingLogic.h"

MissHandlingLogic::MissHandlingLogic(const sc_module_name& name, ComponentID id) :
    Component(name, id) {

  iRequestFromBanks.init(MEMS_PER_TILE);
  oResponseToBanks.init(MEMS_PER_TILE); // Only one of these - broadcast to all?

}

