/*
 * DataNetwork.cpp
 *
 *  Created on: 15 Jul 2014
 *      Author: db434
 */

#include "DataNetwork.h"

DataNetwork::DataNetwork(const sc_module_name &name, size2d_t size,
                         const router_parameters_t& routerParams) :
    Mesh(name, size, TILE, routerParams) {


}

DataNetwork::~DataNetwork() {
  // Nothing
}
