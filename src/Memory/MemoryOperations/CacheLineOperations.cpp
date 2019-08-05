/*
 * CacheLineOperations.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#include "CacheLineOperations.h"
#include "../../Datatype/Instruction.h"
#include "../../Tile/Memory/MemoryBank.h"
#include "../../Utility/Instrumentation/L1Cache.h"
#include "../../Utility/Parameters.h"

#include <assert.h>

MetadataOperation::MetadataOperation(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    MemoryOperation(address, metadata, returnAddr, MEMORY_METADATA,
                    ALIGN_CACHE_LINE, 1, false, false) {
  // Nothing
}

uint MetadataOperation::payloadFlitsRemaining() const {
  return 0;
}

uint MetadataOperation::resultFlitsRemaining() const {
  return 0;
}


AllLinesOperation::AllLinesOperation(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    MemoryOperation(0, metadata, returnAddr, MEMORY_CACHE_LINE,
                    ALIGN_CACHE_LINE, 0, false, false) {
  // Note that address is ignored above. The address is only used to select a
  // memory bank.
}

void AllLinesOperation::assignToMemory(MemoryBase& memory, MemoryLevel level) {
  MemoryOperation::assignToMemory(memory, level);
  sramAddress = 0;

  // Extract extra information about the memory. These operations are only valid
  // on memory banks (i.e. not main memory).
  totalIterations = static_cast<MemoryBank&>(memory).numCacheLines();
}

void AllLinesOperation::prepare() {
  // Nothing to do
}

bool AllLinesOperation::preconditionsMet() const {
  return true;
}

uint AllLinesOperation::payloadFlitsRemaining() const {
  return 0;
}

uint AllLinesOperation::resultFlitsRemaining() const {
  return 0;
}


FetchLine::FetchLine(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    LoadOperation(address, metadata, returnAddr, MEMORY_WORD, ALIGN_CACHE_LINE,
                  CACHE_LINE_WORDS) {
  // Nothing
}


IPKRead::IPKRead(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    LoadOperation(address, metadata, returnAddr, MEMORY_INSTRUCTION, ALIGN_WORD,
                  CACHE_LINE_WORDS - ((address >> 2) & 7)) {
  // The above is a messy way of computing how many words are left in the
  // current cache line. The operation will stop there if it has not encountered
  // an instruction with the end-of-packet marker.
}


ValidateLine::ValidateLine(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    MetadataOperation(address, metadata, returnAddr) {
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


PrefetchLine::PrefetchLine(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    LoadOperation(address, metadata, ChannelID(), MEMORY_WORD, ALIGN_CACHE_LINE,
                  CACHE_LINE_WORDS) {
  // Nothing
}

void PrefetchLine::execute() {
  // Nothing left to do
}

bool PrefetchLine::complete() const {
  return preconditionsMet();  // Data is in the cache
}


FlushLine::FlushLine(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    MetadataOperation(address, metadata, returnAddr) {
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


InvalidateLine::InvalidateLine(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    MetadataOperation(address, metadata, returnAddr) {
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


FlushAllLines::FlushAllLines(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    AllLinesOperation(address, metadata, returnAddr) {
  // Nothing
}

bool FlushAllLines::oneIteration() {
  flushLine();
  return true;
}


InvalidateAllLines::InvalidateAllLines(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    AllLinesOperation(address, metadata, returnAddr) {
  // Nothing
}

bool InvalidateAllLines::oneIteration() {
  invalidateLine();
  return true;
}


StoreLine::StoreLine(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    StoreOperation(address, metadata, returnAddr, MEMORY_WORD, ALIGN_CACHE_LINE,
                   CACHE_LINE_WORDS) {
  // Nothing
}


MemsetLine::MemsetLine(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    StoreLine(address, metadata, returnAddr) {
  data = 0;
  haveData = false;
}

bool MemsetLine::payloadAvailable() const {
  return haveData || StoreLine::payloadAvailable();
}

unsigned int MemsetLine::getPayload() {
  // Only get a payload once, then reuse it until the operation finishes.
  if (!haveData) {
    data = StoreLine::getPayload();
    haveData = true;
  }

  return data;
}


PushLine::PushLine(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    StoreLine(address, metadata, returnAddr) {
  // Target bank is encoded in the address, but we don't know the encoding until
  // we have access to a memory bank. This process is completed in
  // assignToMemory.
  targetBank = address;
}

void PushLine::assignToMemory(MemoryBase& memory, MemoryLevel level) {
  StoreLine::assignToMemory(memory, level);

  // It only makes sense to push lines into a MemoryBank (i.e. not main
  // memory), so this is safe.
  MemoryBank& bank = static_cast<MemoryBank&>(memory);

  // Target bank is encoded in the space where the cache line offset usually
  // goes.
  targetBank = targetBank & (bank.memoriesThisTile() - 1);
}

NetworkRequest PushLine::toFlit() const {
  return NetworkRequest(address+targetBank, ChannelID(), metadata.flatten());
}
