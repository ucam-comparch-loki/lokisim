/*
 * CacheLineOperations.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#include "CacheLineOperations.h"

#include <assert.h>

#include "../Instruction.h"
#include "../../Tile/Memory/MemoryBank.h"
#include "../../Utility/Instrumentation/MemoryBank.h"
#include "../../Utility/Parameters.h"

FetchLine::FetchLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 0, CACHE_LINE_WORDS, CACHE_LINE_BYTES) {
  lineCursor = 0;
}

void FetchLine::prepare() {
  allocateLine();
}

bool FetchLine::preconditionsMet() const {
  return inCache();
}

void FetchLine::execute() {
  assert(preconditionsMet());
  unsigned int result = memory.readWord(sramAddress + lineCursor, getAccessMode());
  memory.printOperation(metadata.opcode, address + lineCursor, result);
  sendResult(result);

  if (level != MEMORY_OFF_CHIP)
    Instrumentation::MemoryBank::continueOperation(memory.id, metadata.opcode, address + lineCursor, false, destination);

  lineCursor += BYTES_PER_WORD;
}


IPKRead::IPKRead(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 0, CACHE_LINE_WORDS, BYTES_PER_WORD) {
  lineCursor = 0;
}

void IPKRead::prepare() {
  allocateLine();
}

bool IPKRead::preconditionsMet() const {
  return inCache();
}

void IPKRead::execute() {
  assert(preconditionsMet());
  unsigned int result = memory.readWord(sramAddress + lineCursor, getAccessMode());
  memory.printOperation(metadata.opcode, address + lineCursor, result);
  sendResult(result, true);

  if (level != MEMORY_OFF_CHIP)
    Instrumentation::MemoryBank::continueOperation(memory.id, metadata.opcode, address + lineCursor, false, destination);

  lineCursor += BYTES_PER_WORD;

  // Terminate the request if we hit the end of the instruction packet or
  // the end of the cache line.
  if (Instruction(result).endOfPacket() || (memory.getOffset(address+lineCursor) == 0))
    resultFlits = 0;
}


ValidateLine::ValidateLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 0, 0, CACHE_LINE_BYTES) {
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
    MemoryOperation(request, memory, level, destination, 0, 0, CACHE_LINE_BYTES) {
}

void PrefetchLine::prepare() {
  allocateLine();
}

bool PrefetchLine::preconditionsMet() const {
  return inCache();
}

void PrefetchLine::execute() {
  // Nothing left to do
}


FlushLine::FlushLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 0, 0, CACHE_LINE_BYTES) {
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
    MemoryOperation(request, memory, level, destination, 0, 0, CACHE_LINE_BYTES) {
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
    MemoryOperation(request, memory, level, destination, 0, 0) {
  line = 0;
}

void FlushAllLines::prepare() {
  // Nothing to do
}

bool FlushAllLines::preconditionsMet() const {
  return true;
}

void FlushAllLines::execute() {
  sramAddress = line * CACHE_LINE_BYTES;
  flushLine();
  line++;
}

bool FlushAllLines::complete() const {
  return (line >= CACHE_LINES_PER_BANK);
}


InvalidateAllLines::InvalidateAllLines(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 0, 0) {
  line = 0;
}

void InvalidateAllLines::prepare() {
  // Nothing to do
}

bool InvalidateAllLines::preconditionsMet() const {
  return true;
}

void InvalidateAllLines::execute() {
  sramAddress = line * CACHE_LINE_BYTES;
  invalidateLine();
  line++;
}

bool InvalidateAllLines::complete() const {
  return (line >= CACHE_LINES_PER_BANK);
}


StoreLine::StoreLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, CACHE_LINE_WORDS, 0, CACHE_LINE_BYTES) {
  lineCursor = 0;
  preWriteCheck();

}

void StoreLine::prepare() {
  validateLine();
}

bool StoreLine::preconditionsMet() const {
  return inCache();
}

void StoreLine::execute() {
  assert(preconditionsMet());

  if (payloadAvailable()) {
    unsigned int data = getPayload();
    memory.writeWord(sramAddress + lineCursor, data, getAccessMode());
    memory.printOperation(metadata.opcode, address + lineCursor, data);

    if (level != MEMORY_OFF_CHIP)
      Instrumentation::MemoryBank::continueOperation(memory.id, metadata.opcode, address + lineCursor, false, destination);

    lineCursor += BYTES_PER_WORD;
  }
}


MemsetLine::MemsetLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 1, 0, CACHE_LINE_BYTES) {
  data = 0;
  lineCursor = 0;

  preWriteCheck();
}

void MemsetLine::prepare() {
  validateLine();
}

bool MemsetLine::preconditionsMet() const {
  return inCache();
}

void MemsetLine::execute() {
  assert(preconditionsMet());

  if (awaitingPayload() && payloadAvailable())
    data = getPayload();

  if (!awaitingPayload()) {
    memory.writeWord(sramAddress + lineCursor, data, getAccessMode());
    memory.printOperation(metadata.opcode, address + lineCursor, data);

    if (level != MEMORY_OFF_CHIP)
      Instrumentation::MemoryBank::continueOperation(memory.id, metadata.opcode, address + lineCursor, false, destination);

    lineCursor += BYTES_PER_WORD;
  }
}

bool MemsetLine::complete() const {
  return (lineCursor >= CACHE_LINE_BYTES);
}


PushLine::PushLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, CACHE_LINE_WORDS, 0, CACHE_LINE_BYTES) {
  lineCursor = 0;
  targetBank = address & (MEMS_PER_TILE - 1);
}

void PushLine::prepare() {
  validateLine();
}

bool PushLine::preconditionsMet() const {
  return inCache();
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

    if (level != MEMORY_OFF_CHIP)
      Instrumentation::MemoryBank::continueOperation(memory.id, metadata.opcode, address + lineCursor, false, destination);

    lineCursor += BYTES_PER_WORD;
  }
}
