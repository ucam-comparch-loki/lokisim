/*
 * ForwardCrossbar.cpp
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#include "ForwardCrossbar.h"

ForwardCrossbar::ForwardCrossbar(const sc_module_name name, ComponentID tile) :
    Crossbar(name, tile, CORES_PER_TILE, MEMS_PER_TILE, 1, Network::COMPONENT, 1){
  // All initialisation done in constructor.

}

ForwardCrossbar::~ForwardCrossbar() {
  // Nothing
}

