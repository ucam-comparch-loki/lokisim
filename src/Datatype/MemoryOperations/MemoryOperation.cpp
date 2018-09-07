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

#include "../../Network/NetworkTypes.h"
#include "../../Tile/Memory/MemoryBank.h"
#include "../../Utility/Arguments.h"
#include "../../Utility/Instrumentation/L1Cache.h"
#include "../../Utility/Instrumentation/Latency.h"

void checkAlignment(MemoryAddr address, uint alignment) {
  if (WARN_UNALIGNED && (address & (alignment-1)) != 0)
    LOKI_WARN << "attempting to access address " << LOKI_HEX(address)
        << " with alignment " << alignment << "." << endl;
}

MemoryOperation::MemoryOperation(const NetworkRequest& request,
                                 MemoryBase& memory,
                                 MemoryLevel level,
                                 ChannelID destination,
                                 unsigned int payloadFlits,
                                 unsigned int maxResultFlits,
                                 unsigned int alignment) :
    request(request),
    address(request.payload().toUInt() & ~(alignment-1)),
    metadata(request.getMemoryMetadata()),
    memory(memory),
    level(level),
    destination(destination),
    payloadFlits(payloadFlits),
    resultFlits(maxResultFlits),
    sramAddress(memory.getPosition(address, getAccessMode())),
    cacheMiss(false) {

//  checkAlignment(request.payload().toUInt(), alignment);

  if (level != MEMORY_OFF_CHIP) {
    MemoryBank& bank = static_cast<MemoryBank&>(memory);

    Instrumentation::L1Cache::startOperation(bank,
                                             metadata.opcode,
                                             address,
                                             !inCache(),
                                             destination);

    if (Arguments::csimTrace())
      cout << "MEM" << bank.globalMemoryIndex() << " "
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
    memory.preWriteCheck(*this);
}

MemoryAccessMode MemoryOperation::getAccessMode() const {
  if (metadata.scratchpad || (level == MEMORY_OFF_CHIP))
    return MEMORY_SCRATCHPAD;
  else
    return MEMORY_CACHE;
}

bool MemoryOperation::payloadAvailable() const {
  return memory.payloadAvailable(level);
}

unsigned int MemoryOperation::getPayload() {
  assert(payloadAvailable());
  payloadFlits--;
  return memory.getPayload(level);
}

void MemoryOperation::sendResult(unsigned int data, bool isInstruction) {
  NetworkResponse response(data, destination, true);
  response.setInstruction(isInstruction);
  memory.sendResponse(response, level);

  Instrumentation::Latency::memoryBufferedResult(memory.id, getOriginal(),
      response, !wasCacheMiss(), getMemoryLevel() == MEMORY_L1);

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

const NetworkRequest MemoryOperation::getOriginal() const {return request;}
MemoryAddr      MemoryOperation::getAddress()     const {return address;}
SRAMAddress     MemoryOperation::getSRAMAddress() const {return sramAddress;}
MemoryMetadata  MemoryOperation::getMetadata()    const {return metadata;}
MemoryLevel     MemoryOperation::getMemoryLevel() const {return level;}
ChannelID       MemoryOperation::getDestination() const {return destination;}

void            MemoryOperation::notifyCacheMiss()      {cacheMiss = true;}
bool            MemoryOperation::wasCacheMiss()   const {return cacheMiss;}

string MemoryOperation::toString() const {
  std::ostringstream out;
  out << memoryOpName(metadata.opcode) << " " << LOKI_HEX(address) << " for " << destination.getString();
  return out.str();
}

MemoryAddr MemoryOperation::startOfLine(MemoryAddr address) const {
  return address - memory.getOffset(address);
}
