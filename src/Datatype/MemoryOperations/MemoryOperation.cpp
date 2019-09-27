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

uint MemoryOperation::operationCount = 0;

MemoryOperation::MemoryOperation(MemoryAddr address,
                                 MemoryMetadata metadata,
                                 ChannelID returnAddress,
                                 MemoryData datatype,
                                 MemoryAlignment alignment,
                                 uint iterations,
                                 bool reads,
                                 bool writes) :
    id(operationCount++),
    address(align(address, alignment)),
    metadata(metadata),
    returnAddress(returnAddress),
    totalIterations(iterations),
    datatype(datatype),
    dataSize(getSize(datatype)),
    alignment(alignment),
    readsMemory(reads),
    writesMemory(writes) {

  sramAddress = -1;
  memory = NULL;
  level = MEMORY_L1; // Add "unknown" option?
  cacheMiss = false;
  endOfPacketSeen = false;

  iterationsComplete = 0;
  addressOffset = 0;

}

MemoryOperation::~MemoryOperation() {}

void MemoryOperation::assignToMemory(MemoryBase& memory, MemoryLevel level) {
  this->memory = &memory;
  this->level = level;
  this->sramAddress = memory.getPosition(address, getAccessMode());

  // Now we have the memory, we can find out the cache line length.
  if (datatype == MEMORY_CACHE_LINE)
    dataSize = memory.cacheLineBytes();

  if (level != MEMORY_OFF_CHIP) {
    MemoryBank& bank = static_cast<MemoryBank&>(memory);

    Instrumentation::L1Cache::startOperation(bank,
                                             metadata.opcode,
                                             address,
                                             !inCache(),
                                             returnAddress);

    if (Arguments::csimTrace())
      cout << "MEM" << bank.globalMemoryIndex() << " "
           << "0x" << std::setfill('0') << std::setw(8) << std::hex << address << std::dec << " "
           << memoryOpName(metadata.opcode) << " "
           << (inCache() ? "hit" : "miss") << endl;
  }

  // Make sure we're allowed to write to this address.
  if (writesMemory)
    preWriteCheck();
}

bool MemoryOperation::needsForwarding() const {
  assert(memoryAssigned());
  return (level == MEMORY_L1 && metadata.skipL1)
      || (level == MEMORY_L2 && metadata.skipL2);
}

void MemoryOperation::forwardResult(unsigned int data, bool isInstruction) {
  sendResult(data, isInstruction);

  // Assume that one flit corresponds to one iteration. This can be overridden
  // in subclasses if necessary.
  iterationsComplete++;
}

void MemoryOperation::prepare() {
  // If we access any data, we need to allocate the cache line and bring it into
  // cache. The only exception is when we overwrite the whole line. In that
  // situation, we can just clear a space.

  bool writingWholeLine = (dataSize * totalIterations == CACHE_LINE_BYTES)
                       && writesMemory;

  if (writingWholeLine && !readsMemory)
    validateLine();
  else if (writesMemory || readsMemory)
    allocateLine();
}

void MemoryOperation::execute() {
  assert(preconditionsMet());

  bool success = oneIteration();

  if (success) {
    // Continue the stats collection which started in `assignToMemory`.
    if (iterationsComplete > 0 && level != MEMORY_OFF_CHIP) {
      MemoryBank& bank = static_cast<MemoryBank&>(*memory);
      Instrumentation::L1Cache::continueOperation(bank, metadata.opcode,
          getAddress(), false, returnAddress);
    }

    iterationsComplete++;
    addressOffset += dataSize;
  }
}

bool MemoryOperation::complete() const {
  return preconditionsMet() &&
      (endOfPacketSeen || (iterationsComplete == totalIterations));
}

bool MemoryOperation::awaitingPayload() const {
  return payloadFlitsRemaining() > 0;
}

bool MemoryOperation::resultsToSend() const {
  return resultFlitsRemaining() > 0;
}

void MemoryOperation::preWriteCheck() const {
  assert(memoryAssigned());
  if (getAccessMode() != MEMORY_SCRATCHPAD)
    memory->preWriteCheck(*this);
}

MemoryAccessMode MemoryOperation::getAccessMode() const {
  assert(memoryAssigned());
  if (metadata.scratchpad || (level == MEMORY_OFF_CHIP))
    return MEMORY_SCRATCHPAD;
  else
    return MEMORY_CACHE;
}

bool MemoryOperation::oneIteration() {
  assert(false);
  return false;
}

uint32_t MemoryOperation::readMemory(bool magic) {
  assert(readsMemory);

  SRAMAddress fullAddress = getSRAMAddress();
  uint32_t result = memory->readWord(fullAddress & ~0x3, getAccessMode(), magic);

  // If we want less than a full word, mask and shift the piece we want.
  if (dataSize < 4) {
    uint offset = (fullAddress & 0x3) / dataSize;
    uint32_t mask = (1 << (dataSize*8)) - 1;
    result = (result >> (offset * dataSize * 8)) & mask;
  }

  if (datatype == MEMORY_INSTRUCTION && Instruction(result).endOfPacket())
    endOfPacketSeen = true;

  memory->printOperation(metadata.opcode, getAddress(), result);
  return result;
}

void MemoryOperation::writeMemory(uint32_t data, bool magic) {
  assert(writesMemory);

  SRAMAddress fullAddress = getSRAMAddress();

  // If writing less than a full word, read the rest of the word and insert
  // this piece before writing it back.
  if (dataSize < 4) {
    uint32_t oldData = memory->readWord(fullAddress & ~0x3, getAccessMode(), true);
    uint offset = (fullAddress & 0x3) / dataSize;

    uint32_t mask = (1 << (dataSize*8)) - 1;
    mask <<= offset * dataSize * 8;

    data = (~mask & oldData) | (mask & (data << (8 * dataSize * offset)));
  }

  memory->printOperation(metadata.opcode, getAddress(), data);
  memory->writeWord(fullAddress & ~0x3, data, getAccessMode(), magic);
}

bool MemoryOperation::payloadAvailable() const {
  assert(memoryAssigned());
  return memory->payloadAvailable(level);
}

unsigned int MemoryOperation::getPayload() {
  assert(payloadAvailable());
  assert(awaitingPayload());
  return memory->getPayload(level);
}

void MemoryOperation::sendResult(unsigned int data, bool isInstruction) {
  assert(memoryAssigned());
  assert(resultsToSend());

  // Do nothing if we don't have a return address.
  // This is useful for e.g. prefetching, or atomic updates where we don't care
  // about the old value.
  if (returnAddress.isNullMapping())
    return;

  bool endOfPacket = (resultFlitsRemaining() == 1) || endOfPacketSeen;

  Word payload = isInstruction ? Instruction(data) : Word(data);

  NetworkResponse response(payload, returnAddress, endOfPacket);
  response.setInstruction(isInstruction);
  memory->sendResponse(response, level);

  Instrumentation::Latency::memoryBufferedResult(memory->id, *this,
      response, !wasCacheMiss(), getMemoryLevel() == MEMORY_L1);
}

void MemoryOperation::makeReservation() {
  memory->makeReservation(returnAddress.component, getAddress(),
                          getAccessMode());
}

bool MemoryOperation::checkReservation() const {
  return memory->checkReservation(returnAddress.component, getAddress(),
                                  getAccessMode());
}

void MemoryOperation::allocateLine() const {
  memory->allocate(getAddress(), getSRAMAddress(), getAccessMode());
}

void MemoryOperation::validateLine() const {
  memory->validate(getAddress(), getSRAMAddress(), getAccessMode());
}

void MemoryOperation::invalidateLine() const {
  memory->invalidate(getSRAMAddress(), getAccessMode());
}

void MemoryOperation::flushLine() const {
  memory->flush(getSRAMAddress(), getAccessMode());
}

bool MemoryOperation::inCache() const {
  // Using getAddress and getSRAMAddress breaks this - bug or not?
  return memory->contains(address, sramAddress, getAccessMode());
}

MemoryAddr      MemoryOperation::getAddress()     const {
  return address + addressOffset;
}

SRAMAddress     MemoryOperation::getSRAMAddress() const {
  return sramAddress + addressOffset;
}

MemoryMetadata  MemoryOperation::getMetadata()    const {return metadata;}
MemoryLevel     MemoryOperation::getMemoryLevel() const {return level;}
ChannelID       MemoryOperation::getDestination() const {return returnAddress;}

void            MemoryOperation::notifyCacheMiss()      {cacheMiss = true;}
bool            MemoryOperation::wasCacheMiss()   const {return cacheMiss;}

string MemoryOperation::toString() const {
  std::ostringstream out;
  out << memoryOpName(metadata.opcode) << " " << LOKI_HEX(address) << " for "
      << returnAddress.getString(Encoding::hardwareChannelID);

  return out.str();
}

NetworkRequest MemoryOperation::toFlit() const {
  return NetworkRequest(address, returnAddress, metadata.flatten());
}

bool MemoryOperation::memoryAssigned() const {
  return memory != NULL;
}

MemoryAddr MemoryOperation::align(MemoryAddr address,
                                  MemoryAlignment alignment) const {
  // TODO: would prefer to do this when we have access to `memory`, so different
  // memories can offer different alignment options.

  size_t bytes;
  switch (alignment) {
    case ALIGN_BYTE:       bytes = 1;                break;
    case ALIGN_HALFWORD:   bytes = BYTES_PER_WORD/2; break;
    case ALIGN_WORD:       bytes = BYTES_PER_WORD;   break;
    case ALIGN_CACHE_LINE: bytes = CACHE_LINE_BYTES; break;
    default: assert(false); bytes = -1; break;
  }

  return address & ~(bytes - 1);
}


size_t MemoryOperation::getSize(MemoryData datatype) const {
  switch (datatype) {
    case MEMORY_BYTE:        return 1;
    case MEMORY_HALFWORD:    return BYTES_PER_WORD/2;
    case MEMORY_WORD:        return BYTES_PER_WORD;
    case MEMORY_INSTRUCTION: return BYTES_PER_WORD;
    case MEMORY_CACHE_LINE:  return 0; // Fixed up in `assignMemory`
    case MEMORY_METADATA:    return 0;
    default: assert(false); return -1;
  }
}
