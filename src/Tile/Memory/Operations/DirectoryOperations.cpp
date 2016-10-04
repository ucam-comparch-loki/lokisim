/*
 * DirectoryOperations.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#include "../../../Tile/Memory/Operations/DirectoryOperations.h"

#include <assert.h>

UpdateDirectoryEntry::UpdateDirectoryEntry(MemoryAddr address, MemoryMetadata metadata, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(address, metadata, memory, level, destination, 1, 0) {
  // Nothing
}

bool UpdateDirectoryEntry::needsForwarding() const {
  // This operation takes place at the directory, not the memory bank, so
  // always forward it on.
  return true;
}

void UpdateDirectoryEntry::prepare() {
  assert(false);
}

bool UpdateDirectoryEntry::preconditionsMet() const {
  return false;
}

void UpdateDirectoryEntry::execute() {
  assert(false);
}


UpdateDirectoryMask::UpdateDirectoryMask(MemoryAddr address, MemoryMetadata metadata, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(address, metadata, memory, level, destination, 1, 0) {
  // Nothing
}

bool UpdateDirectoryMask::needsForwarding() const {
  // This operation takes place at the directory, not the memory bank, so
  // always forward it on.
  return true;
}

void UpdateDirectoryMask::prepare() {
  assert(false);
}

bool UpdateDirectoryMask::preconditionsMet() const {
  return false;
}

void UpdateDirectoryMask::execute() {
  assert(false);
}
