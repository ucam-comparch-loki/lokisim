/*
 * CacheLineOperations.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#include "CacheLineOperations.h"

#include <assert.h>

#include "../../../Datatype/Instruction.h"
#include "../../../Utility/Instrumentation/MemoryBank.h"
#include "../../../Utility/Parameters.h"
#include "../MemoryBank.h"

FetchLine::FetchLine(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(startOfLine(address), metadata, memory, level, destination, 0, CACHE_LINE_WORDS) {
  lineCursor = 0;
  if (ENERGY_TRACE)
    Instrumentation::MemoryBank::initiateBurstRead(memory.id.globalMemoryNumber());
}

void FetchLine::prepare() {
  allocateLine();
}

bool FetchLine::preconditionsMet() const {
  return inCache();
}

void FetchLine::execute() {
  assert(preconditionsMet());
  unsigned int result = memory.readWord(sramAddress + lineCursor);
  memory.printOperation(metadata.opcode, address + lineCursor, result);
  sendResult(result);

  lineCursor += BYTES_PER_WORD;
}


IPKRead::IPKRead(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(address, metadata, memory, level, destination, 0, CACHE_LINE_WORDS) {
  lineCursor = 0;
  if (ENERGY_TRACE)
    Instrumentation::MemoryBank::initiateIPKRead(memory.id.globalMemoryNumber());
}

void IPKRead::prepare() {
  allocateLine();
}

bool IPKRead::preconditionsMet() const {
  return inCache();
}

void IPKRead::execute() {
  assert(preconditionsMet());
  unsigned int result = memory.readWord(sramAddress + lineCursor);
  memory.printOperation(metadata.opcode, address + lineCursor, result);
  sendResult(result);

  lineCursor += BYTES_PER_WORD;

  // Terminate the request if we hit the end of the instruction packet or
  // the end of the cache line.
  if (Instruction(result).endOfPacket() || (memory.getOffset(address+lineCursor) == 0))
    resultFlits = 0;
}


ValidateLine::ValidateLine(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(startOfLine(address), metadata, memory, level, destination, 0, 0) {
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


PrefetchLine::PrefetchLine(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(startOfLine(address), metadata, memory, level, destination, 0, 0) {
  if (ENERGY_TRACE)
    Instrumentation::MemoryBank::initiateBurstRead(memory.id.globalMemoryNumber());
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


FlushLine::FlushLine(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(startOfLine(address), metadata, memory, level, destination, 0, 0) {
  finished = false;
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


InvalidateLine::InvalidateLine(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(startOfLine(address), metadata, memory, level, destination, 0, 0) {
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


FlushAllLines::FlushAllLines(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(address, metadata, memory, level, destination, 0, 0) {
  line = 0;
}

void FlushAllLines::prepare() {
  // Nothing to do
}

bool FlushAllLines::preconditionsMet() const {
  return true;
}

void FlushAllLines::execute() {
  if (ENERGY_TRACE)
    Instrumentation::MemoryBank::initiateBurstRead(memory.id.globalMemoryNumber());
  sramAddress = line * CACHE_LINE_BYTES;
  flushLine();
  line++;
}

bool FlushAllLines::complete() const {
  return (line >= CACHE_LINES_PER_BANK);
}


InvalidateAllLines::InvalidateAllLines(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(address, metadata, memory, level, destination, 0, 0) {
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


StoreLine::StoreLine(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(startOfLine(address), metadata, memory, level, destination, CACHE_LINE_WORDS, 0) {
  lineCursor = 0;
  preWriteCheck();
  if (ENERGY_TRACE)
    Instrumentation::MemoryBank::initiateBurstWrite(memory.id.globalMemoryNumber());

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
    memory.writeWord(sramAddress + lineCursor, data);
    memory.printOperation(metadata.opcode, address + lineCursor, data);

    lineCursor += BYTES_PER_WORD;
  }
}


MemsetLine::MemsetLine(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(startOfLine(address), metadata, memory, level, destination, 1, 0) {
  data = 0;
  lineCursor = 0;

  preWriteCheck();

  if (ENERGY_TRACE)
    Instrumentation::MemoryBank::initiateBurstWrite(memory.id.globalMemoryNumber());
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
    memory.writeWord(sramAddress + lineCursor, data);
    memory.printOperation(metadata.opcode, address + lineCursor, data);

    lineCursor += BYTES_PER_WORD;
  }
}

bool MemsetLine::complete() const {
  return (lineCursor >= CACHE_LINE_BYTES);
}


PushLine::PushLine(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(startOfLine(address), metadata, memory, level, destination, CACHE_LINE_WORDS, 0) {
  lineCursor = 0;
}

void PushLine::prepare() {
  validateLine();
}

bool PushLine::preconditionsMet() const {
  return inCache();
}

void PushLine::execute() {
  assert(preconditionsMet());

  if (payloadAvailable()) {
    unsigned int data = getPayload();
    memory.writeWord(sramAddress + lineCursor, data);
    memory.printOperation(metadata.opcode, address + lineCursor, data);

    lineCursor += BYTES_PER_WORD;
  }
}
