/*
 * DirectoryOperations.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#include "DirectoryOperations.h"

#include <assert.h>

UpdateDirectoryEntry::UpdateDirectoryEntry(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 1, 0) {
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

const NetworkRequest UpdateDirectoryEntry::getOriginal() const {
  // Since the MHL doesn't know whether to check the Skip L1 or Skip L2 bit when
  // it receives a directory update request, copy whichever bit is appropriate
  // into the Scratchpad bit. This bit is always overwritten when forwarding
  // the request.
  MemoryMetadata meta = metadata;
  meta.scratchpad = MemoryOperation::needsForwarding();
  return NetworkRequest(address, destination, meta.flatten());
}


UpdateDirectoryMask::UpdateDirectoryMask(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 1, 0) {
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

const NetworkRequest UpdateDirectoryMask::getOriginal() const {
  // Since the MHL doesn't know whether to check the Skip L1 or Skip L2 bit when
  // it receives a directory update request, copy whichever bit is appropriate
  // into the Scratchpad bit. This bit is always overwritten when forwarding
  // the request.
  MemoryMetadata meta = metadata;
  meta.scratchpad = MemoryOperation::needsForwarding();
  return NetworkRequest(address, destination, meta.flatten());
}
