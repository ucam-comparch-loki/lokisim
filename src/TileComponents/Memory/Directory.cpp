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

void Directory::setEntry(unsigned int entry, unsigned int tile) {
  assert(tile < NUM_TILES);
  assert(entry < directory.size());

  directory[entry] = tile;
}

void Directory::initialise(unsigned int tile) {
  assert(tile < NUM_TILES);

  directory.assign(directory.size(), tile);
}

unsigned int Directory::getTile(MemoryAddr address) const {
  unsigned int entry = (address >> shiftAmount) & bitmask;
  return directory[entry];
}
