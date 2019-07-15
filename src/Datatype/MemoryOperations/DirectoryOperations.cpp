/*
 * DirectoryOperations.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#include "DirectoryOperations.h"

#include <assert.h>

DirectoryOperation::DirectoryOperation(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination, unsigned int payloadFlits) :
    MemoryOperation(request, memory, level, destination, payloadFlits, 0) {
  // Nothing
}

bool DirectoryOperation::needsForwarding() const {
  // These operations take place at the directory, not the memory bank, so
  // always forward.
  return true;
}

void DirectoryOperation::prepare() {
  assert(false);
}

bool DirectoryOperation::preconditionsMet() const {
  return false;
}

void DirectoryOperation::execute() {
  assert(false);
}

const NetworkRequest DirectoryOperation::getOriginal() const {
  // Since the MHL doesn't know whether to check the Skip L1 or Skip L2 bit when
  // it receives a directory update request, copy whichever bit is appropriate
  // into the Scratchpad bit. This bit is always overwritten when forwarding
  // the request.
  MemoryMetadata meta = metadata;
  meta.scratchpad = MemoryOperation::needsForwarding();
  return NetworkRequest(address, destination, meta.flatten());
}


UpdateDirectoryEntry::UpdateDirectoryEntry(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    DirectoryOperation(request, memory, level, destination, 1) {
  // Nothing
}


UpdateDirectoryMask::UpdateDirectoryMask(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    DirectoryOperation(request, memory, level, destination, 1) {
  // Nothing
}
