/*
 * Directory.cpp
 *
 *  Created on: 24 Jun 2014
 *      Author: db434
 */

#include "Directory.h"
#include <assert.h>
#include "../../Utility/Parameters.h"

Directory::Directory(unsigned int size) :
    bitmask(size-1),
    directory(size) {

  assert((size & (size-1)) == 0); // Need power of 2.

  shiftAmount = 0;

}

void Directory::setBitmaskLSB(unsigned int lsb) {
  assert(lsb < 32);

  shiftAmount = lsb;
}

void Directory::setEntry(unsigned int entry, TileIndex tile) {
  uint column = (tile >> 3) & 7;
  uint row = tile & 7;
  assert(column < TOTAL_TILE_COLUMNS);
  assert(row < TOTAL_TILE_ROWS);
  assert(entry < directory.size());

  directory[entry] = tile;
}

void Directory::initialise(TileIndex tile) {
  uint column = (tile >> 3) & 7;
  uint row = tile & 7;
  assert(column < TOTAL_TILE_COLUMNS);
  assert(row < TOTAL_TILE_ROWS);

  directory.assign(directory.size(), tile);
}

TileIndex Directory::getTile(MemoryAddr address) const {
  unsigned int entry = (address >> shiftAmount) & bitmask;
  return directory[entry];
}
