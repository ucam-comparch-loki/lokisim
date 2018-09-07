/*
 * ForwardCrossbar.cpp
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#include "ForwardCrossbar.h"

ForwardCrossbar::ForwardCrossbar(const sc_module_name name, ComponentID tile,
                                 const tile_parameters_t& params) :
    Crossbar(name, tile, params.numCores, params.numMemories, 1, Network::COMPONENT, 1){
  // All initialisation done in constructor.

}

ForwardCrossbar::~ForwardCrossbar() {
  // Nothing
}

