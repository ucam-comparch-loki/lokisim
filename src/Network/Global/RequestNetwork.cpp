/*
 * RequestNetwork.cpp
 *
 *  Created on: 15 Jul 2014
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "RequestNetwork.h"

RequestNetwork::RequestNetwork(const sc_module_name &name) :
    Mesh(name, ComponentID(), TOTAL_TILE_ROWS, TOTAL_TILE_COLUMNS, TILE) {
  // Nothing
}

RequestNetwork::~RequestNetwork() {
  // Nothing
}

