/*
 * CacheLineOperations.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#include "CacheLineOperations.h"
#include "../Instruction.h"
#include "../../Tile/Memory/MemoryBank.h"
#include "../../Utility/Instrumentation/L1Cache.h"
#include "../../Utility/Parameters.h"

#include <assert.h>

CacheLineOperation::CacheLineOperation(const NetworkRequest& request,
                                       MemoryBase& memory,
                                       MemoryLevel level,
                                       ChannelID destination,
                                       unsigned int payloadFlits,
                                       unsigned int maxResultFlits,
                                       unsigned int alignment) :
    MemoryOperation(request, memory, level, destination, payloadFlits, maxResultFlits, alignment) {
  lineCursor = 0;
}


ReadLineOperation::ReadLineOperation(const NetworkRequest& request,
                                     MemoryBase& memory,
                                     MemoryLevel level,
                                     ChannelID destination,
                                     unsigned int alignment) :
    CacheLineOperation(request, memory, level, destination, 0, memory.cacheLineWords(), alignment) {
  // Nothing
}

void ReadLineOperation::prepare() {
  allocateLine();
}

bool ReadLineOperation::preconditionsMet() const {
  return inCache();
}


WriteLineOperation::WriteLineOperation(const NetworkRequest& request,
                                       MemoryBase& memory,
                                       MemoryLevel level,
                                       ChannelID destination,
                                       unsigned int payloadFlits) :
    CacheLineOperation(request, memory, level, destination, payloadFlits, 0, memory.cacheLineBytes()) {
  preWriteCheck();
}

void WriteLineOperation::prepare() {
  // Overwriting the whole line, so no need for allocation.
  validateLine();
}

bool WriteLineOperation::preconditionsMet() const {
  return inCache();
}


MetadataOperation::MetadataOperation(const NetworkRequest& request,
                                     MemoryBase& memory,
                                     MemoryLevel level,
                                     ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 0, 0, 1) {
  // Nothing
}


FetchLine::FetchLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    ReadLineOperation(request, memory, level, destination, memory.cacheLineBytes()) {
  // Nothing
}

void FetchLine::execute() {
  assert(preconditionsMet());
  unsigned int result = memory.readWord(sramAddress + lineCursor, getAccessMode());
  memory.printOperation(metadata.opcode, address + lineCursor, result);
  sendResult(result);

  if (level != MEMORY_OFF_CHIP) {
    MemoryBank& bank = static_cast<MemoryBank&>(memory);
    Instrumentation::L1Cache::continueOperation(bank, metadata.opcode, address + lineCursor, false, destination);
  }

  lineCursor += BYTES_PER_WORD;
}


IPKRead::IPKRead(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    ReadLineOperation(request, memory, level, destination, BYTES_PER_WORD) {
  // Nothing
}

void IPKRead::execute() {
  assert(preconditionsMet());
  unsigned int result = memory.readWord(sramAddress + lineCursor, getAccessMode());
  memory.printOperation(metadata.opcode, address + lineCursor, result);
  sendResult(result, true);

  if (level != MEMORY_OFF_CHIP) {
    MemoryBank& bank = static_cast<MemoryBank&>(memory);
    Instrumentation::L1Cache::continueOperation(bank, metadata.opcode, address + lineCursor, false, destination);
  }

  lineCursor += BYTES_PER_WORD;

  // Terminate the request if we hit the end of the instruction packet or
  // the end of the cache line.
  if (Instruction(result).endOfPacket() || (memory.getOffset(address+lineCursor) == 0))
    resultFlits = 0;
}


ValidateLine::ValidateLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MetadataOperation(request, memory, level, destination) {
  // Nothing
}

void ValidateLine::prepare() {
  validateLine();
}

bool ValidateLine::preconditionsMet() const {
  return inCache();
}

void ValidateLine::execute() {
  // Nothing left to do
}


PrefetchLine::PrefetchLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    ReadLineOperation(request, memory, level, destination, memory.cacheLineBytes()) {
}

void PrefetchLine::execute() {
  // Nothing left to do
}

bool PrefetchLine::complete() const {
  return preconditionsMet();  // Data is in the cache
}


FlushLine::FlushLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MetadataOperation(request, memory, level, destination) {
  finished = false;

  // Instrumentation happens in the memory bank. At this point, we don't know
  // if the data is dirty and needs flushing. There are also other mechanisms
  // which can trigger a flush.
}

void FlushLine::prepare() {
  // Nothing to do
}

bool FlushLine::preconditionsMet() const {
  return true;
}

void FlushLine::execute() {
  if (inCache())
    flushLine();

  finished = true;
}

bool FlushLine::complete() const {
  return finished;
}


InvalidateLine::InvalidateLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MetadataOperation(request, memory, level, destination) {
  finished = false;
}

void InvalidateLine::prepare() {
  // Nothing to do
}

bool InvalidateLine::preconditionsMet() const {
  return true;
}

void InvalidateLine::execute() {
  if (inCache())
    invalidateLine();

  finished = true;
}

bool InvalidateLine::complete() const {
  return finished;
}


FlushAllLines::FlushAllLines(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MetadataOperation(request, memory, level, destination) {
  line = 0;
}

void FlushAllLines::prepare() {
  // Nothing to do
}

bool FlushAllLines::preconditionsMet() const {
  return true;
}

void FlushAllLines::execute() {
  sramAddress = line * memory.cacheLineBytes();
  flushLine();
  line++;
}

bool FlushAllLines::complete() const {
  // It only makes sense to flush lines from a MemoryBank (i.e. not main
  // memory), so this is safe.
  MemoryBank& bank = static_cast<MemoryBank&>(memory);

  return (line >= bank.numCacheLines());
}


InvalidateAllLines::InvalidateAllLines(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MetadataOperation(request, memory, level, destination) {
  line = 0;
}

void InvalidateAllLines::prepare() {
  // Nothing to do
}

bool InvalidateAllLines::preconditionsMet() const {
  return true;
}

void InvalidateAllLines::execute() {
  sramAddress = line * memory.cacheLineBytes();
  invalidateLine();
  line++;
}

bool InvalidateAllLines::complete() const {
  // It only makes sense to invalidate lines in a MemoryBank (i.e. not main
  // memory), so this is safe.
  MemoryBank& bank = static_cast<MemoryBank&>(memory);

  return (line >= bank.numCacheLines());
}


StoreLine::StoreLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    WriteLineOperation(request, memory, level, destination, memory.cacheLineWords()) {
  // Nothing
}

void StoreLine::execute() {
  assert(preconditionsMet());

  if (payloadAvailable()) {
    unsigned int data = getPayload();
    memory.writeWord(sramAddress + lineCursor, data, getAccessMode());
    memory.printOperation(metadata.opcode, address + lineCursor, data);

    if (level != MEMORY_OFF_CHIP) {
      MemoryBank& bank = static_cast<MemoryBank&>(memory);
      Instrumentation::L1Cache::continueOperation(bank, metadata.opcode, address + lineCursor, false, destination);
    }

    lineCursor += BYTES_PER_WORD;
  }
}


MemsetLine::MemsetLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    WriteLineOperation(request, memory, level, destination, 1) {
  data = 0;
}

void MemsetLine::execute() {
  assert(preconditionsMet());

  if (awaitingPayload() && payloadAvailable())
    data = getPayload();

  if (!awaitingPayload()) {
    memory.writeWord(sramAddress + lineCursor, data, getAccessMode());
    memory.printOperation(metadata.opcode, address + lineCursor, data);

    if (level != MEMORY_OFF_CHIP) {
      MemoryBank& bank = static_cast<MemoryBank&>(memory);
      Instrumentation::L1Cache::continueOperation(bank, metadata.opcode, address + lineCursor, false, destination);
    }

    lineCursor += BYTES_PER_WORD;
  }
}

bool MemsetLine::complete() const {
  return (lineCursor >= memory.cacheLineBytes());
}


PushLine::PushLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    WriteLineOperation(request, memory, level, destination, memory.cacheLineWords()) {
  // It only makes sense to push lines into a MemoryBank (i.e. not main
  // memory), so this is safe.
  MemoryBank& bank = static_cast<MemoryBank&>(memory);

  // Target bank is encoded in the space where the cache line offset usually
  // goes.
  targetBank = request.payload().toUInt() & (bank.memoriesThisTile() - 1);
}

const NetworkRequest PushLine::getOriginal() const {
  return NetworkRequest(address+targetBank, ChannelID(), metadata.flatten());
}

void PushLine::execute() {
  assert(preconditionsMet());

  if (payloadAvailable()) {
    unsigned int data = getPayload();
    memory.writeWord(sramAddress + lineCursor, data, getAccessMode());
    memory.printOperation(metadata.opcode, address + lineCursor, data);

    if (level != MEMORY_OFF_CHIP) {
      MemoryBank& bank = static_cast<MemoryBank&>(memory);
      Instrumentation::L1Cache::continueOperation(bank, metadata.opcode, address + lineCursor, false, destination);
    }

    lineCursor += BYTES_PER_WORD;
  }
}
