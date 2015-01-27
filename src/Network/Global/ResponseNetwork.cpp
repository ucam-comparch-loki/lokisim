/*
 * ResponseNetwork.cpp
 *
 *  Created on: 15 Jul 2014
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "ResponseNetwork.h"

ResponseNetwork::ResponseNetwork(const sc_module_name &name) :
    NetworkHierarchy2(name, 1, 1, 1) {
  // Nothing
}

ResponseNetwork::~ResponseNetwork() {
  // Nothing
}

