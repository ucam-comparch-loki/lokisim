/*
 * EmptyTile.cpp
 *
 *  Created on: 5 Oct 2016
 *      Author: db434
 */

#include "EmptyTile.h"

EmptyTile::EmptyTile(const sc_module_name& name, const ComponentID& id) :
    Tile(name, id) {
  // TODO Set up dead-end connections

}

EmptyTile::~EmptyTile() {
  // Nothing
}

