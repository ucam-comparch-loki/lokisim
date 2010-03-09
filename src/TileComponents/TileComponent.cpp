/*
 * TileComponent.cpp
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#include "TileComponent.h"

/* Constructors and destructors */
TileComponent::TileComponent(sc_module_name name, int ID) : Component(name, ID) {

  flowControlOut = new sc_out<bool>[NUM_CLUSTER_INPUTS];
  in             = new sc_in<Word>[NUM_CLUSTER_INPUTS];

  flowControlIn  = new sc_in<bool>[NUM_CLUSTER_OUTPUTS];
  out            = new sc_out<AddressedWord>[NUM_CLUSTER_OUTPUTS];

}

TileComponent::~TileComponent() {

}
