/*
 * TileComponent.cpp
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#include "TileComponent.h"

void TileComponent::print(int start, int end) const {
  // Do nothing if print isn't defined
}

Word TileComponent::getMemVal(int addr) const {
  return Word(-1);
}

int TileComponent::getRegVal(int reg) const {
  return -1;
}

int TileComponent::getInstIndex() const {
  return -1;
}

bool TileComponent::getPredReg() const {
  return false;
}

/* Constructors and destructors */
TileComponent::TileComponent(sc_module_name name, int ID) : Component(name, ID) {

  flowControlOut = new sc_out<int>[NUM_CLUSTER_INPUTS];
  in             = new sc_in<Word>[NUM_CLUSTER_INPUTS];

  flowControlIn  = new sc_in<bool>[NUM_CLUSTER_OUTPUTS];
  out            = new sc_out<AddressedWord>[NUM_CLUSTER_OUTPUTS];

}

TileComponent::~TileComponent() {

}
