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

  assert(size > 0);
  assert((size & (size-1)) == 0); // Need power of 2.

  shiftAmount = 0;

}

void Directory::setBitmaskLSB(unsigned int lsb) {
  assert(lsb < 32);

  shiftAmount = lsb;
}

void Directory::setEntry(unsigned int entry, unsigned int data) {
  assert(entry < directory.size());
  directory[entry] = DirectoryEntry(data);
}

void Directory::initialise(TileID tile) {
  for (unsigned int i=0; i<directory.size(); i++) {
    DirectoryEntry entry;
    entry.tile = tile;
    entry.replaceBits = i; // Replacement index bits = input index bits
    directory[i] = entry;
  }
}

unsigned int Directory::getEntry(MemoryAddr address) const {
  return (address >> shiftAmount) & bitmask;
}

TileID Directory::getTile(MemoryAddr address) const {
  unsigned int entry = getEntry(address);
  return directory[entry].tile;
}

MemoryAddr Directory::updateAddress(MemoryAddr address) const {
  unsigned int entry = getEntry(address);
  return (address & ~(bitmask << shiftAmount))
       | (directory[entry].replaceBits << shiftAmount);
}
