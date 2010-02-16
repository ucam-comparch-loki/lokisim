/*
 * TileComponent.cpp
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#include "TileComponent.h"

/* Constructors and destructors */
TileComponent::TileComponent(sc_core::sc_module_name name, int ID)
    : Component(name, ID) {

  flowControlIn = new sc_in<bool>[NUM_CLUSTER_OUTPUTS];
  out = new sc_out<AddressedWord>[NUM_CLUSTER_OUTPUTS];

}

TileComponent::~TileComponent() {

}


// Copy constructor
TileComponent::TileComponent(const TileComponent& other) : Component(other) {
  id = other.id;
}

// Assignment operator
TileComponent* TileComponent::operator= (const TileComponent& other) {
  return new TileComponent(other);
}
