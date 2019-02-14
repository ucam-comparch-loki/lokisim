/*
 * DataReturn.cpp
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#include "DataReturn.h"

DataReturn::DataReturn(const sc_module_name name,
                       const tile_parameters_t& params) :
    Crossbar(name, params.numMemories, params.numCores, 1, params.core.numInputChannels) {
  // All initialisation handled by Crossbar.

}

DataReturn::~DataReturn() {
  // Nothing
}

