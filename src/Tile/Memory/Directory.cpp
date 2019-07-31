/*
 * Directory.cpp
 *
 *  Created on: 24 Jun 2014
 *      Author: db434
 */

#include "Directory.h"

#include <ostream>

#include "../../Utility/Assert.h"
#include "../../Utility/Logging.h"

using std::endl;


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

  LOKI_LOG(1) << "Directory updated bitmask to start at bit " << lsb << endl;
}

void Directory::setEntry(unsigned int entry, unsigned int data) {
  assert(entry < directory.size());
  directory[entry] = DirectoryEntry(data);

  LOKI_LOG(1) << "Directory updated entry " << entry << " to point to tile "
      << TileID(data, Encoding::softwareTileID) << endl;
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
  unsigned int entry = (address >> shiftAmount) & bitmask;
  assert(entry < directory.size());
  return entry;
}

TileID Directory::getNextTile(MemoryAddr address) const {
  unsigned int entry = getEntry(address);
  return directory[entry].tile;
}

bool Directory::inScratchpad(MemoryAddr address) const {
  unsigned int entry = getEntry(address);
  return directory[entry].scratchpad;
}

MemoryAddr Directory::updateAddress(MemoryAddr address) const {
  unsigned int entry = getEntry(address);
  return (address & ~(bitmask << shiftAmount))
       | (directory[entry].replaceBits << shiftAmount);
}

NetworkRequest Directory::updateRequest(const NetworkRequest& request) const {
  // We need the head flit of a packet, or else we won't have a memory address
  // to use to access the directory.
  MemoryMetadata metadata = request.getMemoryMetadata();
  assert((metadata.opcode != PAYLOAD) && (metadata.opcode != PAYLOAD_EOP));

  MemoryAddr address = request.payload().toUInt();
  unsigned int entry = getEntry(address);

  TileID nextTile = directory[entry].tile;
  ChannelID destination(nextTile.x, nextTile.y, request.channelID().component.position, request.channelID().channel);
  address = (address & ~(bitmask << shiftAmount))
          | (directory[entry].replaceBits << shiftAmount);
  metadata.scratchpad = directory[entry].scratchpad;

  NetworkRequest updated = request;
  updated.setChannelID(destination);
  updated.setPayload(address);
  updated.setMetadata(metadata.flatten());
  return updated;
}
