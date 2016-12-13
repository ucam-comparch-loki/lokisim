/*
 * CoreMulticast.cpp
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#include "CoreMulticast.h"

CoreMulticast::CoreMulticast(const sc_module_name name, ComponentID tile)
    : MulticastNetwork(name, tile, CORES_PER_TILE, CORES_PER_TILE*CORE_INPUT_CHANNELS, CORE_INPUT_CHANNELS, CORE_INPUT_CHANNELS){
  // All initialisation handled by MulticastNetwork.

}

CoreMulticast::~CoreMulticast() {
  // Nothing.
}

