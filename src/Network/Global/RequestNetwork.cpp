/*
 * RequestNetwork.cpp
 *
 *  Created on: 15 Jul 2014
 *      Author: db434
 */

#include "RequestNetwork.h"
#include "../../TileComponents/Memory/MemoryBank.h"

RequestNetwork::RequestNetwork(const sc_module_name &name) :
    NetworkHierarchy2(name, NUM_MEMORIES, 1) {

}

RequestNetwork::~RequestNetwork() {
  // Nothing
}

