/*
 * MemoryOperation.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#include "MemoryOperation.h"

#include <assert.h>
#include <iomanip>
#include <iostream>

#include "../../../Network/NetworkTypedefs.h"
#include "../../../Utility/Arguments.h"
#include "../../../Utility/Instrumentation/MemoryBank.h"
#include "../MemoryBank.h"

MemoryOperation::MemoryOperation(MemoryAddr address,
                                 MemoryMetadata metadata,
                                 MemoryBase& memory,
                                 MemoryLevel level,
                                 ChannelID destination,
                                 unsigned int payloadFlits,
                                 unsigned int maxResultFlits) :
    address(address),
    metadata(metadata),
    memory(memory),
    level(level),
    destination(destination),
    payloadFlits(payloadFlits),
    resultFlits(maxResultFlits),
    sramAddress(memory.getPosition(address, getAccessMode())) {

  if (level != MEMORY_OFF_CHIP) {
    Instrumentation::MemoryBank::startOperation(memory.id.globalMemoryNumber(),
                                                metadata.opcode,
                                                address,
                                                !inCache(),
                                                destination);

    if (Arguments::csimTrace())
      cout << "MEM" << memory.id.globalMemoryNumber() << " "
           << "0x" << std::setfill('0') << std::setw(8) << std::hex << address << std::dec << " "
           << memoryOpName(metadata.opcode) << " "
           << (inCache() ? "hit" : "miss") << endl;
  }

}

MemoryOperation::~MemoryOperation() {}

bool MemoryOperation::needsForwarding() const {
  return (level == MEMORY_L1 && metadata.skipL1)
      || (level == MEMORY_L2 && metadata.skipL2);
}

bool MemoryOperation::complete() const {
  return preconditionsMet() && !awaitingPayload() && !resultsToSend();
}

bool MemoryOperation::awaitingPayload() const {
  return payloadFlits > 0;
}

bool MemoryOperation::resultsToSend() const {
  return resultFlits > 0;
}

void MemoryOperation::preWriteCheck() const {
  if (getAccessMode() != MEMORY_SCRATCHPAD)
    memory.preWriteCheck(address);
}

MemoryAccessMode MemoryOperation::getAccessMode() const {
  switch (level) {
    case MEMORY_L1:
      if (metadata.scratchpadL1)
        return MEMORY_SCRATCHPAD;
      else
        return MEMORY_CACHE;

    case MEMORY_L2:
      if (metadata.scratchpadL2)
        return MEMORY_SCRATCHPAD;
      else
        return MEMORY_CACHE;

    case MEMORY_OFF_CHIP:
      return MEMORY_SCRATCHPAD;
  }

  assert(false);
  return MEMORY_SCRATCHPAD;
}

bool MemoryOperation::payloadAvailable() const {
  return memory.payloadAvailable(level);
}

unsigned int MemoryOperation::getPayload() {
  assert(payloadAvailable());
  payloadFlits--;
  return memory.getPayload(level);
}

void MemoryOperation::sendResult(unsigned int data) {
  NetworkResponse response(data, destination, true);
  memory.sendResponse(response, level);
  resultFlits--;
}

void MemoryOperation::allocateLine() const {
  memory.allocate(address, sramAddress, getAccessMode());
}

void MemoryOperation::validateLine() const {
  memory.validate(address, sramAddress, getAccessMode());
}

void MemoryOperation::invalidateLine() const {
  memory.invalidate(sramAddress, getAccessMode());
}

void MemoryOperation::flushLine() const {
  memory.flush(sramAddress, getAccessMode());
}

bool MemoryOperation::inCache() const {
  return memory.contains(address, sramAddress, getAccessMode());
}

NetworkRequest MemoryOperation::getOriginal() const {
  return NetworkRequest(address, ChannelID(), metadata.flatten());
}

MemoryAddr      MemoryOperation::getAddress()     const {return address;}
SRAMAddress     MemoryOperation::getSRAMAddress() const {return sramAddress;}
MemoryMetadata  MemoryOperation::getMetadata()    const {return metadata;}
MemoryLevel     MemoryOperation::getMemoryLevel() const {return level;}
ChannelID       MemoryOperation::getDestination() const {return destination;}

MemoryAddr MemoryOperation::startOfLine(MemoryAddr address) const {
  return address - memory.getOffset(address);
}
