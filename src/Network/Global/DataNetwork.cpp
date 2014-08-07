/*
 * DataNetwork.cpp
 *
 *  Created on: 15 Jul 2014
 *      Author: db434
 */

#include "DataNetwork.h"
#include "../../TileComponents/Core.h"

DataNetwork::DataNetwork(const sc_module_name &name) :
    NetworkHierarchy2(name, NUM_CORES, CORE_INPUT_CHANNELS) {


}

DataNetwork::~DataNetwork() {
  // Nothing
}
