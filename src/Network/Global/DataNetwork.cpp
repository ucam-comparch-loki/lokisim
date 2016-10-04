/*
 * DataNetwork.cpp
 *
 *  Created on: 15 Jul 2014
 *      Author: db434
 */

#include "DataNetwork.h"
#include "../../Tile/Core/Core.h"

DataNetwork::DataNetwork(const sc_module_name &name) :
    NetworkHierarchy2(name, CORES_PER_TILE, CORES_PER_TILE, CORE_INPUT_CHANNELS) {


}

DataNetwork::~DataNetwork() {
  // Nothing
}
