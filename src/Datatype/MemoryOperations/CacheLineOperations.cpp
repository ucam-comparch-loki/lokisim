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

MetadataOperation::MetadataOperation(const NetworkRequest& request,
                                     ChannelID destination) :
    MemoryOperation(request.payload().toUInt(), request.getMemoryMetadata(),
                    destination, MEMORY_METADATA, ALIGN_CACHE_LINE, 1, false, false) {
  // Nothing
}

uint MetadataOperation::payloadFlitsRemaining() const {
  return 0;
}

uint MetadataOperation::resultFlitsRemaining() const {
  return 0;
}


FetchLine::FetchLine(const NetworkRequest& request, ChannelID destination) :
    LoadOperation(request.payload().toUInt(), request.getMemoryMetadata(),
                  destination, MEMORY_WORD, ALIGN_CACHE_LINE, CACHE_LINE_WORDS) {
  // Nothing
}


IPKRead::IPKRead(const NetworkRequest& request, ChannelID destination) :
    LoadOperation(request.payload().toUInt(), request.getMemoryMetadata(),
                  destination, MEMORY_INSTRUCTION, ALIGN_WORD,
                  CACHE_LINE_WORDS - ((request.payload().toUInt() >> 2) & 7)) {
  // The above is a messy way of computing how many words are left in the
  // current cache line. The operation will stop there if it has not encountered
  // an instruction with the end-of-packet marker.
}


ValidateLine::ValidateLine(const NetworkRequest& request, ChannelID destination) :
    MetadataOperation(request, destination) {
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


PrefetchLine::PrefetchLine(const NetworkRequest& request, ChannelID destination) :
    LoadOperation(request.payload().toUInt(), request.getMemoryMetadata(),
                  ChannelID(), MEMORY_WORD, ALIGN_CACHE_LINE, CACHE_LINE_WORDS) {
  // Ignore the given return address - there is no result to produce.
}

void PrefetchLine::execute() {
  // Nothing left to do
}

bool PrefetchLine::complete() const {
  return preconditionsMet();  // Data is in the cache
}


FlushLine::FlushLine(const NetworkRequest& request, ChannelID destination) :
    MetadataOperation(request, destination) {
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


InvalidateLine::InvalidateLine(const NetworkRequest& request, ChannelID destination) :
    MetadataOperation(request, destination) {
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


FlushAllLines::FlushAllLines(const NetworkRequest& request, ChannelID destination) :
    MetadataOperation(request, destination) {
  line = 0;
}

void FlushAllLines::prepare() {
  // Nothing to do
}

bool FlushAllLines::preconditionsMet() const {
  return true;
}

void FlushAllLines::execute() {
  sramAddress = line * memory->cacheLineBytes();
  flushLine();
  line++;
}

bool FlushAllLines::complete() const {
  // It only makes sense to flush lines from a MemoryBank (i.e. not main
  // memory), so this is safe.
  MemoryBank& bank = static_cast<MemoryBank&>(*memory);

  return (line >= bank.numCacheLines());
}


InvalidateAllLines::InvalidateAllLines(const NetworkRequest& request, ChannelID destination) :
    MetadataOperation(request, destination) {
  line = 0;
}

void InvalidateAllLines::prepare() {
  // Nothing to do
}

bool InvalidateAllLines::preconditionsMet() const {
  return true;
}

void InvalidateAllLines::execute() {
  sramAddress = line * memory->cacheLineBytes();
  invalidateLine();
  line++;
}

bool InvalidateAllLines::complete() const {
  // It only makes sense to invalidate lines in a MemoryBank (i.e. not main
  // memory), so this is safe.
  MemoryBank& bank = static_cast<MemoryBank&>(*memory);

  return (line >= bank.numCacheLines());
}


StoreLine::StoreLine(const NetworkRequest& request, ChannelID destination) :
    StoreOperation(request.payload().toUInt(), request.getMemoryMetadata(),
                   destination, MEMORY_WORD, ALIGN_CACHE_LINE, CACHE_LINE_WORDS) {
  // Nothing
}


MemsetLine::MemsetLine(const NetworkRequest& request, ChannelID destination) :
    StoreLine(request, destination) {
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


PushLine::PushLine(const NetworkRequest& request, ChannelID destination) :
    StoreLine(request, destination) {
  // It only makes sense to push lines into a MemoryBank (i.e. not main
  // memory), so this is safe.
  MemoryBank& bank = static_cast<MemoryBank&>(*memory);

  // Target bank is encoded in the space where the cache line offset usually
  // goes.
  targetBank = request.payload().toUInt() & (bank.memoriesThisTile() - 1);
}

NetworkRequest PushLine::toFlit() const {
  return NetworkRequest(address+targetBank, ChannelID(), metadata.flatten());
}
