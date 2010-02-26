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

  // Connect all the tiles together vertically
  for(int i=1; i<rows; i++) {       // Start at 1 because we subtract 1
    for(int j=0; j<columns; j++) {
//      const Tile &top = tiles.get(i-1, j);
//      const Tile &bottom = tiles.get(i,j);
      Tile::connectTopBottom(tiles.get(i-1, j), tiles.get(i,j));
    }
  }

  // Connect all the tiles together horizontally
  for(int i=0; i<rows; i++) {
    for(int j=1; j<columns; j++) {  // Start at 1 because we subtract 1
//      const Tile &left = tiles.get(i, j-1);
//      const Tile &right = tiles.get(i,j);
      Tile::connectLeftRight(tiles.get(i, j-1), tiles.get(i,j));
    }
  }

  // TODO: What happens at the edge of the chip?
  //       Connect up as a torus for now?
}

Chip::~Chip() {

}
