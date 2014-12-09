/*
 * CreditNetwork.cpp
 *
 *  Created on: 15 Jul 2014
 *      Author: db434
 */

#include "CreditNetwork.h"
#include "../../TileComponents/Core.h"

CreditNetwork::CreditNetwork(const sc_module_name &name) :
    NetworkHierarchy2(name, CORES_PER_TILE, CORES_PER_TILE, 1) {

}

CreditNetwork::~CreditNetwork() {
  // Nothing
}

