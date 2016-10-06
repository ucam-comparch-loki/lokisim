/*
 * DataNetwork.cpp
 *
 *  Created on: 15 Jul 2014
 *      Author: db434
 */

#include "DataNetwork.h"

DataNetwork::DataNetwork(const sc_module_name &name) :
    Mesh(name, ComponentID(), TOTAL_TILE_ROWS, TOTAL_TILE_COLUMNS, TILE) {


}

DataNetwork::~DataNetwork() {
  // Nothing
}
