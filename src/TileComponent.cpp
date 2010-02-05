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
