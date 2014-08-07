/*
 * ResponseNetwork.cpp
 *
 *  Created on: 15 Jul 2014
 *      Author: db434
 */

#include "ResponseNetwork.h"
#include "../../TileComponents/Memory/MemoryBank.h"

ResponseNetwork::ResponseNetwork(const sc_module_name &name) :
    NetworkHierarchy2(name, NUM_MEMORIES, 1) {

}

ResponseNetwork::~ResponseNetwork() {
  // Nothing
}

