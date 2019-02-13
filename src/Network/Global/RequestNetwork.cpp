/*
 * RequestNetwork.cpp
 *
 *  Created on: 15 Jul 2014
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "RequestNetwork.h"

RequestNetwork::RequestNetwork(const sc_module_name &name, size2d_t size,
                               const router_parameters_t& routerParams) :
    Mesh(name, size, TILE, routerParams) {
  // Nothing
}

RequestNetwork::~RequestNetwork() {
  // Nothing
}

