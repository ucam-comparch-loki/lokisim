/*
 * DirectoryOperations.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#include "DirectoryOperations.h"

#include <assert.h>

DirectoryOperation::DirectoryOperation(const NetworkRequest& request, ChannelID destination, unsigned int payloadFlits) :
    MemoryOperation(request.payload().toUInt(), request.getMemoryMetadata(),
                    destination, MEMORY_METADATA, ALIGN_BYTE, 1, false, false) {
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
  return true;
}

void DirectoryOperation::execute() {
  assert(false);
}

uint DirectoryOperation::payloadFlitsRemaining() const {
  return 1;
}

uint DirectoryOperation::resultFlitsRemaining() const {
  return 0;
}

NetworkRequest DirectoryOperation::toFlit() const {
  // Since the MHL doesn't know whether to check the Skip L1 or Skip L2 bit when
  // it receives a directory update request, copy whichever bit is appropriate
  // into the Scratchpad bit. This bit is always overwritten when forwarding
  // the request.
  MemoryMetadata meta = metadata;
  meta.scratchpad = MemoryOperation::needsForwarding();
  return NetworkRequest(address, returnAddress, meta.flatten());
}


UpdateDirectoryEntry::UpdateDirectoryEntry(const NetworkRequest& request, ChannelID destination) :
    DirectoryOperation(request, destination, 1) {
  // Nothing
}


UpdateDirectoryMask::UpdateDirectoryMask(const NetworkRequest& request, ChannelID destination) :
    DirectoryOperation(request, destination, 1) {
  // Nothing
}
