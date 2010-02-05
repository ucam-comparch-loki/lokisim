/*
 * Chip.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "Chip.h"

Chip::Chip(sc_core::sc_module_name name, int rows, int columns)
    : Component(name) {
  tiles = new Grid<Tile>(rows, columns);

  // Connect all the tiles together vertically
  for(int i=1; i<rows; i++) {       // Start at 1 because we subtract 1
    for(int j=0; j<columns; j++) {
      Tile* top = tiles->get(i-1, j);
      Tile* bottom = tiles->get(i,j);
      Tile::connectTopBottom(*top, *bottom);
    }
  }

  // Connect all the tiles together horizontally
  for(int i=0; i<rows; i++) {
    for(int j=1; j<columns; j++) {  // Start at 1 because we subtract 1
      Tile* left = tiles->get(i, j-1);
      Tile* right = tiles->get(i,j);
      Tile::connectLeftRight(*left, *right);
    }
  }

  // TODO: What happens at the edge of the chip?
}

Chip::Chip(const Chip& other) : Component(other) {
  tiles = other.tiles;
}

Chip::~Chip() {
  delete tiles;
}
