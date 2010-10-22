/*
 * Chip.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "Chip.h"

Chip::Chip(sc_core::sc_module_name name, int rows, int columns) :
    Component(name, 0),
    tiles(rows, columns) {

  // Create all the tiles
  for(int i=0; i<rows; i++) {
    for(int j=0; j<columns; j++) {
      Tile *t = new Tile("tile", i*rows + j);
      tiles.initialPut(i, *t);
    }
  }

  // TODO: What happens at the edge of the chip?
  //       Connect up as a torus for now?
}

Chip::~Chip() {

}
