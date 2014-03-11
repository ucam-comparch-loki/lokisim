/*
 * IPKCacheBase.cpp
 *
 *  Created on: 11 Jul 2012
 *      Author: db434
 */

#include "IPKCacheBase.h"
#include "../Utility/Instrumentation.h"
#include "../Utility/Instrumentation/IPKCache.h"

using std::cerr;
using std::cout;
using std::endl;

IPKCacheBase::IPKCacheBase(const size_t size, const size_t numTags, const std::string& name) :
    name(name),
    tags(numTags, DEFAULT_TAG),
    data(size),
    fresh(size, false),
    readPointer(size),
    writePointer(size) {

  writePointer.setNull();
  readPointer = 0;

  jumpAmount = 0;

  fillCount = 0;

  finishedPacketRead = true;
  finishedPacketWrite = true;
  lastOpWasARead = true;

  lastTagActivity = lastDataActivity = -1;

}

IPKCacheBase::~IPKCacheBase() {
  // Do nothing
}

CacheIndex IPKCacheBase::lookup(const MemoryAddr tag) {
  tagActivity();
  return cacheIndex(tag);
}

bool IPKCacheBase::packetExists(CacheIndex position) const {
  return getTag(position) != DEFAULT_TAG;
}

const Instruction IPKCacheBase::read() {
  if (!finishedPacketRead)
    updateReadPointer();
  assert(!readPointer.isNull());

  const Instruction inst = data[readPointer.value()];
  dataActivity();
  fillCount--;

  if (fresh[readPointer.value()]) {
    dataConsumed.notify();
    fresh[readPointer.value()] = false;
  }

  finishedPacketRead = inst.endOfPacket();
  lastOpWasARead = true;
  return inst;
}

CacheIndex IPKCacheBase::write(const Instruction newData) {
  updateWritePointer();
  assert(!writePointer.isNull());

  // Write the data to the cache.
  data[writePointer.value()] = newData;
  fresh[writePointer.value()] = true;
  dataActivity();

  setTag(writePointer.value(), DEFAULT_TAG);

  lastOpWasARead = false;

  finishedPacketWrite = newData.endOfPacket();

  return writePointer.value();
}

void IPKCacheBase::setTag(MemoryAddr tag) {
  if (ENERGY_TRACE) {
    // FIXME: currently setting whole tags to 0xFFFFFFFF instead of using a
    // single invalidation bit. This messes with the recorded Hamming distances.

    const MemoryAddr oldTag = getTag(writePointer.value());
    Instrumentation::IPKCache::tagWrite(oldTag, tag);
  }

  setTag(writePointer.value(), tag);
  tagActivity();
}

/* Jump to a new instruction at a given offset. */
void IPKCacheBase::jump(const JumpOffset offset) {
  // Store the offset. The jump will be made next time an instruction is read
  // from the cache.
  jumpAmount = offset;
  lastOpWasARead = false;
  finishedPacketRead = false;
}

/* Returns the remaining number of entries in the cache. */
const size_t IPKCacheBase::remainingSpace() const {
  if (finishedPacketRead)
    return size();
  else
    return size() - fillCount;
}

/* Returns whether the cache is empty. Note that even if a cache is empty,
 * it is still possible to access its contents if an appropriate tag is
 * looked up.
 * This behaviour makes it awkward to tell when the cache is actually empty!
 * Need to take into account whether there are any packets queued up too. */
bool IPKCacheBase::empty() const {
  return (readPointer.value() == writePointer.value()) && lastOpWasARead;
}

/* Returns whether the cache is full. */
bool IPKCacheBase::full() const {
  return fillCount == size();
}

const sc_event& IPKCacheBase::dataConsumedEvent() const {
  return dataConsumed;
}

/* We can issue a new fetch command if there is space in the cache for a
 * maximum-sized instruction packet, and no other fetches are already in
 * progress. */
bool IPKCacheBase::canFetch() const {
  return (remainingSpace() >= MAX_IPK_SIZE);
}

void IPKCacheBase::cancelPacket() {
  finishedPacketRead = true;
}

/* Store some initial instructions in the cache. */
CacheIndex IPKCacheBase::storeCode(const std::vector<Instruction>& code) {
  if (code.size() > size())
    cerr << "Error: tried to write " << code.size() <<
      " instructions to a instruction cache of size " << size() << endl;

  // Write the first instruction separately so we can record where it was
  // written.
  write(code[0]);

  CacheIndex writePos = writePointer.value();

  for (uint i=1; i<code.size() && i<size(); i++)
    write(code[i]);

  return writePos;
}

size_t IPKCacheBase::size()                       const {return data.size();}

CacheIndex IPKCacheBase::getReadPointer()         const {return readPointer.value();}
CacheIndex IPKCacheBase::getWritePointer()        const {return writePointer.value();}
void IPKCacheBase::setReadPointer(CacheIndex pos)       {readPointer = pos; lastOpWasARead = false; updateFillCount();}
void IPKCacheBase::setWritePointer(CacheIndex pos)      {writePointer = pos; updateFillCount();}

void IPKCacheBase::incrementWritePos() {
  ++writePointer;
  fillCount++;

  assert(fillCount <= size());
}

void IPKCacheBase::incrementReadPos() {
  ++readPointer;
//  fillCount--;

  assert(fillCount <= size());
}

void IPKCacheBase::updateFillCount() {
  // Read and write pointers being in the same place could mean that the cache
  // is full or empty. Determine which case it is using the most recent operation.
  if (writePointer == readPointer)
    if (lastOpWasARead)
      fillCount = 0;
    else
      fillCount = size();
  else
    fillCount = writePointer - readPointer;

  // Hack. We have to wait until very late to increment the read pointer, in
  // case there are any jumps to take into account, so there is sometimes one
  // more instruction in the cache than we would expect.
  if (finishedPacketRead && !finishedPacketWrite) {
    fillCount++;
    if (fillCount > size())
      fillCount -= size();
  }
}

void IPKCacheBase::tagActivity() {
  if (!ENERGY_TRACE)
    return;

  cycle_count_t currentCycle = Instrumentation::currentCycle();
  if (currentCycle != lastTagActivity) {
    Instrumentation::IPKCache::tagActivity();
    lastTagActivity = currentCycle;
  }
}

void IPKCacheBase::dataActivity() {
  if (!ENERGY_TRACE)
    return;

  cycle_count_t currentCycle = Instrumentation::currentCycle();
  if (currentCycle != lastDataActivity) {
    Instrumentation::IPKCache::dataActivity();
    lastDataActivity = currentCycle;
  }
}
