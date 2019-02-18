/*
 * IPKCacheBase.cpp
 *
 *  Created on: 11 Jul 2012
 *      Author: db434
 */

#include "IPKCacheBase.h"

#include <systemc>

#include "../Utility/Instrumentation.h"
#include "../Utility/Instrumentation/IPKCache.h"

using std::cerr;
using std::cout;
using std::endl;
using sc_core::sc_event;

IPKCacheBase::IPKCacheBase(const std::string& name, size_t size, size_t numTags,
                           size_t maxIPKLength) :
    name(name),
    maxIPKLength(maxIPKLength),
    tags(numTags, DEFAULT_TAG),
    data(size),
    fresh(size, false),
    readPointer(size),
    writePointer(size) {

  assert(size > 0);
  assert(numTags > 0);

  writePointer.setNull();
  readPointer = 0;

  jumpAmount = 0;

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

const Instruction IPKCacheBase::read() {
  if (!finishedPacketRead)
    updateReadPointer();
  assert(!readPointer.isNull());

  const Instruction inst = data[readPointer.value()];
  dataActivity();

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

  if (fresh[writePointer.value()])
    cerr << "Warning: " << this->name << " overwriting unread instruction\n"
         << "  " << data[writePointer.value()] << " <= " << newData << endl;

  // Write the data to the cache.
  data[writePointer.value()] = newData;
  fresh[writePointer.value()] = true;
  dataActivity();

  lastOpWasARead = false;

  finishedPacketWrite = newData.endOfPacket();

  return writePointer.value();
}

void IPKCacheBase::setTag(MemoryAddr tag) {
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
size_t IPKCacheBase::remainingSpace() const {
  if (finishedPacketRead && finishedPacketWrite)
    return size();
  else
    return size() - getFillCount();
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
  return getFillCount() == size();
}

const sc_event& IPKCacheBase::dataConsumedEvent() const {
  return dataConsumed;
}

/* We can issue a new fetch command if there is space in the cache for a
 * maximum-sized instruction packet, and no other fetches are already in
 * progress. */
bool IPKCacheBase::canFetch() const {
  // Per the Verilog's ipk_cache/ipkcache.v:
  // Need to account for
  // 1. one pending write (e.g. instr about to be written at end of cache line)
  // 2. need to distinguish between full and empty, so assertion checks
  //      wp+1 != rp, so need +1 entries
  return (remainingSpace() > maxIPKLength + 1);
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
void IPKCacheBase::setReadPointer(CacheIndex pos)       {readPointer = pos; lastOpWasARead = false;}
void IPKCacheBase::setWritePointer(CacheIndex pos)      {writePointer = pos;}

void IPKCacheBase::incrementWritePos() {
  ++writePointer;
}

void IPKCacheBase::incrementReadPos() {
  ++readPointer;
}

size_t IPKCacheBase::getFillCount() const {
  size_t fillCount;

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

  return fillCount;
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
