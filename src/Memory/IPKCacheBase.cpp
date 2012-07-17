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
    locations(size),
    readPointer(size),
    writePointer(size) {

  readPointer.setNull();
  writePointer.setNull();

  jumpAmount = 0;

  fillCount = 0;
  currentPacket.reset();
  pendingPacket.reset();

  currentPacket.execute = true;
  finishedPacketRead = false;
  lastOpWasARead = true;

  lastTagActivity = lastDataActivity = -1;

}

bool IPKCacheBase::checkTags(const MemoryAddr address, const opcode_t operation) {
  // Depending on whether are are currently executing a packet or not, we may
  // want to store information in either currentPacket or pendingPacket.
  PacketInfo* packet;
  if (readPointer.isNull()) packet = &currentPacket;
  else                      packet = &pendingPacket;

  // Find out where in the cache the given tag is (if anywhere).
  CacheIndex position = cacheIndex(address);

  packet->cacheIndex = position;
  packet->memAddr    = address;
  packet->inCache    = position != NOT_IN_CACHE;
  packet->execute    = operation != InstructionMap::OP_FILL &&
                       operation != InstructionMap::OP_FILLR;
  packet->persistent = operation == InstructionMap::OP_FETCHPST ||
                       operation == InstructionMap::OP_FETCHPSTR;

  tagActivity();

  return packet->inCache;
}

const Instruction IPKCacheBase::read() {
  updateReadPointer();
  assert(!readPointer.isNull());

  const Instruction inst = data[readPointer.value()];
  dataActivity();

  finishedPacketRead = inst.endOfPacket();
  lastOpWasARead = true;
  return inst;
}

void IPKCacheBase::write(const Instruction newData) {
  updateWritePointer();
  assert(!writePointer.isNull());

  // Determine whether this instruction is part of the packet which is currently
  // executing, or part of the packet due to execute next.
  PacketInfo* packet;
  if (!currentPacket.inCache) packet = &currentPacket;
  else                        packet = &pendingPacket;

  // Write the data to the cache.
  data[writePointer.value()] = newData;
  dataActivity();

  // If this is the first instruction of the packet, tag it with the address
  // from which it was fetched. Also keep track of where in the cache it is
  // going (allows us to jump there when this packet finishes).
  if (packet->cacheIndex == NOT_IN_CACHE) {
    if (ENERGY_TRACE) {
      // FIXME: currently setting whole tags to 0xFFFFFFFF instead of using a
      // single invalidation bit. This messes with the recorded Hamming distances.

      const MemoryAddr oldTag = getTag(writePointer.value());
      const MemoryAddr newTag = packet->memAddr;
      Instrumentation::IPKCache::tagWrite(oldTag, newTag);
    }

    setTag(writePointer.value(), packet->memAddr);
    tagActivity();
    packet->cacheIndex = writePointer.value();
  }
  else
    setTag(writePointer.value(), DEFAULT_TAG);

  // Only the first instruction in each packet has a tag, but we keep track of
  // the locations of all instructions for debug reasons.
  locations[writePointer.value()] = packet->memAddr;
  packet->memAddr += BYTES_PER_WORD;

  // If this is the end of an instruction packet, mark the packet as being
  // in the cache - this may allow the next fetch request to be sent.
  if (newData.endOfPacket()) {
    packet->inCache = true;
    lastOpWasARead = false;
  }

  finishedPacketWrite = newData.endOfPacket();

}

/* Jump to a new instruction at a given offset. */
void IPKCacheBase::jump(const JumpOffset offset) {
  // Store the offset. The jump will be made next time an instruction is read
  // from the cache.
  jumpAmount = offset;
  lastOpWasARead = false;
}

/* Return the memory address of the currently-executing packet. Only returns
 * a useful value for the first instruction in each packet, as this is the
 * only instruction which gets a tag. */
const MemoryAddr IPKCacheBase::packetAddress() const {
  return getTag(currentPacket.cacheIndex);
}

/* Returns the remaining number of entries in the cache. */
const size_t IPKCacheBase::remainingSpace() const {
  if (finishedPacketRead)
    return size();
  else
    return size() - fillCount;
}

const MemoryAddr IPKCacheBase::getInstLocation() const {
  return locations[readPointer.value()];
}

/* Returns whether the cache is empty. Note that even if a cache is empty,
 * it is still possible to access its contents if an appropriate tag is
 * looked up.
 * This behaviour makes it awkward to tell when the cache is actually empty!
 * Need to take into account whether there are any packets queued up too. */
bool IPKCacheBase::empty() const {
  if (currentPacket.cacheIndex == NOT_IN_CACHE)
    return true;
  else if (finishedPacketRead)
    return (pendingPacket.cacheIndex == NOT_IN_CACHE) &&
           (jumpAmount == 0) &&
           !currentPacket.persistent;
  else
    return (fillCount == 0);

  // FIXME: probably also need currentPacket.execute
}

/* Returns whether the cache is full. */
bool IPKCacheBase::full() const {
  return fillCount == size();
}

/* Returns whether this cache is stalled, waiting for instructions to arrive
 * from memory. */
bool IPKCacheBase::stalled() const {
  // Will this work? What if the cache is just plain empty?
  return empty() && currentPacket.arriving();
}

/* We can issue a new fetch command if there is space in the cache for a
 * maximum-sized instruction packet, and no other fetches are already in
 * progress. */
bool IPKCacheBase::canFetch() const {
  return (remainingSpace() >= MAX_IPK_SIZE) &&
      !currentPacket.arriving() && !pendingPacket.arriving();
}

bool IPKCacheBase::packetInProgress() const {
  return currentPacket.memAddr != DEFAULT_TAG && !finishedPacketRead;
}

/* Begin reading the packet which is queued up to execute next. */
void IPKCacheBase::switchToPendingPacket() {

  if (currentPacket.persistent)
    readPointer = currentPacket.cacheIndex;
  else {
    assert(pendingPacket.cacheIndex != NOT_IN_CACHE);
    currentPacket = pendingPacket;
    pendingPacket.reset();

    readPointer = currentPacket.cacheIndex;
  }

  lastOpWasARead = true;

  updateFillCount();

  if (DEBUG) cout << name << " switched to pending packet: current = " <<
      readPointer.value() << ", refill = " << writePointer.value() << endl;
}

void IPKCacheBase::cancelPacket() {
  currentPacket.persistent = false;
  finishedPacketRead = true;

  // Stop the current packet executing if it hasn't finished arriving yet.
  if (currentPacket.arriving())
    currentPacket.execute = false;
}

/* Store some initial instructions in the cache. */
void IPKCacheBase::storeCode(const std::vector<Instruction>& code) {
  if (code.size() > size())
    cerr << "Error: tried to write " << code.size() <<
      " instructions to a instruction cache of size " << size() << endl;

  if (currentPacket.memAddr == DEFAULT_TAG)
    currentPacket.memAddr = 0;

  for (uint i=0; i<code.size() && i<size(); i++)
    write(code[i]);
}

size_t IPKCacheBase::size() const {
  return data.size();
}

bool IPKCacheBase::startOfPacket(CacheIndex position) const {
  // The first instruction of each packet will have a proper tag, not the
  // default.
  return getTag(position) != DEFAULT_TAG;
}

void IPKCacheBase::incrementWritePos() {
  ++writePointer;
  fillCount++;
}

void IPKCacheBase::incrementReadPos() {
  ++readPointer;
  fillCount--;
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
