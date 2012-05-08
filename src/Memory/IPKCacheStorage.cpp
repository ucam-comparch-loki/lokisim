/*
 * IPKCacheStorage.cpp
 *
 *  Created on: 14 Jun 2010
 *      Author: db434
 */

#include "IPKCacheStorage.h"
#include "../Datatype/Instruction.h"
#include "../Utility/Parameters.h"

const MemoryAddr IPKCacheStorage::DEFAULT_TAG;

/* Returns whether the given address matches any of the tags. */
bool IPKCacheStorage::checkTags(const MemoryAddr& key, opcode_t operation) {
  // Depending on whether are are currently executing a packet or not, we may
  // want to store information in either currentPacket or pendingPacket.
  PacketInfo* packet;
  if(readPointer.isNull()) packet = &currentPacket;
  else                     packet = &pendingPacket;

  bool foundMatch = false;

  for(uint i=0; i<this->tags.size(); i++) {
    if(this->tags[i] == key) {  // Found a match
      // We know that the packet starts at this position in the cache.
      packet->cacheIndex = i;

      foundMatch = true;
      break;
    }
  }

  if(!foundMatch) packet->cacheIndex = NOT_IN_CACHE;

  packet->memAddr    = key;
  packet->inCache    = foundMatch;
  packet->execute    = operation != InstructionMap::OP_FILL &&
                       operation != InstructionMap::OP_FILLR;
  packet->persistent = operation == InstructionMap::OP_FETCHPST ||
                       operation == InstructionMap::OP_FETCHPSTR;

  return foundMatch;

}

/* Returns the next item in the cache. */
const Instruction& IPKCacheStorage::read() {

  // Move the read pointer to the next instruction. In the common case, this
  // will just be an increment, but we could also perform an in-buffer jump or
  // switch to a new instruction packet.
  if(jumpAmount != 0) {
    readPointer += jumpAmount;
    jumpAmount = 0;
    updateFillCount();
  }
  else if(finishedPacket)
    switchToPendingPacket();
  else
    incrementReadPos();

  assert(!readPointer.isNull());

  Instruction& inst = this->data_[readPointer.value()];
  finishedPacket = inst.endOfPacket();
  lastOpWasARead = true;

  return inst;
}

/* Writes new data to a position determined using the given key. */
void IPKCacheStorage::write(const Instruction& newData) {

  incrementWritePos();

  // Determine whether this instruction is part of the packet which is currently
  // executing, or part of the packet due to execute next.
  PacketInfo* packet;
  if(!currentPacket.inCache) packet = &currentPacket;
  else                       packet = &pendingPacket;

  // Write the data to the cache.
  this->data_[writePointer.value()] = newData;

  // If this is the first instruction of the packet, tag it with the address
  // from which it was fetched. Also keep track of where in the cache it is
  // going (allows us to jump there when this packet finishes).
  if(packet->cacheIndex == NOT_IN_CACHE) {
    this->tags[writePointer.value()] = packet->memAddr;
    packet->cacheIndex = writePointer.value();
  }
  else this->tags[writePointer.value()] = DEFAULT_TAG;

  // Only the first instruction in each packet has a tag, but we keep track of
  // the locations of all instructions for debug reasons.
  locations[writePointer.value()] = packet->memAddr;
  packet->memAddr += BYTES_PER_WORD;

  // If this is the end of an instruction packet, mark the packet as being
  // in the cache - this may allow the next fetch request to be sent.
  if(newData.endOfPacket()) {
    packet->inCache = true;
    lastOpWasARead = false;
  }
}

/* Jump to a new instruction at a given offset. */
void IPKCacheStorage::jump(const JumpOffset offset) {
  // Store the offset. The jump will be made next time an instruction is read
  // from the cache.
  jumpAmount = offset;
  lastOpWasARead = false;
}

/* Return the memory address of the currently-executing packet. Only returns
 * a useful value for the first instruction in each packet, as this is the
 * only instruction which gets a tag. */
const MemoryAddr IPKCacheStorage::packetAddress() const {
  return this->tags[currentPacket.cacheIndex];
}

/* Returns the remaining number of entries in the cache. */
const uint16_t IPKCacheStorage::remainingSpace() const {
  if(finishedPacket)
    return this->size();
  else
    return this->size() - fillCount;
}

const MemoryAddr IPKCacheStorage::getInstLocation() const {
  return locations[readPointer.value()];
}

/* Returns whether the cache is empty. Note that even if a cache is empty,
 * it is still possible to access its contents if an appropriate tag is
 * looked up.
 * This behaviour makes it awkward to tell when the cache is actually empty!
 * Need to take into account whether there are any packets queued up too. */
bool IPKCacheStorage::empty() const {
  if(currentPacket.cacheIndex == NOT_IN_CACHE)
    return true;
  else if(finishedPacket)
    return (pendingPacket.cacheIndex == NOT_IN_CACHE) &&
           (jumpAmount == 0) &&
           !currentPacket.persistent;
  else
    return (fillCount == 0);

  // FIXME: probably also need currentPacket.execute
}

/* Returns whether the cache is full. */
bool IPKCacheStorage::full() const {
  return fillCount == this->size();
}

/* Returns whether this cache is stalled, waiting for instructions to arrive
 * from memory. */
bool IPKCacheStorage::stalled() const {
  // Will this work? What if the cache is just plain empty?
  return empty() && currentPacket.arriving();
}

/* We can issue a new fetch command if there is space in the cache for a
 * maximum-sized instruction packet, and no other fetches are already in
 * progress. */
bool IPKCacheStorage::canFetch() const {
  return (remainingSpace() >= MAX_IPK_SIZE) &&
      !currentPacket.arriving() && !pendingPacket.arriving();
}

bool IPKCacheStorage::packetInProgress() const {
  return currentPacket.memAddr != DEFAULT_TAG && !finishedPacket;
}

/* Begin reading the packet which is queued up to execute next. */
void IPKCacheStorage::switchToPendingPacket() {

  if(currentPacket.persistent)
    readPointer = currentPacket.cacheIndex;
  else {
    assert(pendingPacket.cacheIndex != NOT_IN_CACHE);
    currentPacket = pendingPacket;
    pendingPacket.reset();

    readPointer = currentPacket.cacheIndex;
  }

  lastOpWasARead = true;

  updateFillCount();

  if(DEBUG) cout << this->name_ << " switched to pending packet: current = " <<
      readPointer.value() << ", refill = " << writePointer.value() << endl;
}

void IPKCacheStorage::cancelPacket() {
  currentPacket.persistent = false;
  finishedPacket = true;

  // Stop the current packet executing if it hasn't finished arriving yet.
  if(currentPacket.arriving())
    currentPacket.execute = false;
}

/* Store some initial instructions in the cache. */
void IPKCacheStorage::storeCode(const std::vector<Instruction>& code) {
  if(code.size() > this->size()) {
    cerr << "Error: tried to write " << code.size() <<
      " instructions to a instruction cache of size " << this->size() << endl;
  }

  for(uint i=0; i<code.size() && i<this->size(); i++) {
    write(code[i]);
  }
}

/* Returns the data corresponding to the given address.
 * Private because we don't want to use this version for IPK caches. */
const Instruction& IPKCacheStorage::read(const MemoryAddr& key) {
  assert(false);
}

/* Returns the position that data with the given address tag should be stored. */
uint16_t IPKCacheStorage::getPosition(const MemoryAddr& key) {
  return writePointer.value();
}

bool IPKCacheStorage::startOfPacket(uint16_t cacheIndex) const {
  // The first instruction of each packet will have a proper tag, not the
  // default.
  return this->tags[cacheIndex] != DEFAULT_TAG;
}

void IPKCacheStorage::incrementWritePos() {
  ++writePointer;
  fillCount++;
}

void IPKCacheStorage::incrementReadPos() {
  ++readPointer;
  fillCount--;
}

void IPKCacheStorage::updateFillCount() {
  // Read and write pointers being in the same place could mean that the cache
  // is full or empty. Determine which case it is using the most recent operation.
  if(writePointer == readPointer)
    if(lastOpWasARead)
      fillCount = 0;
    else
      fillCount = this->size();
  else
    fillCount = writePointer - readPointer;
}

IPKCacheStorage::IPKCacheStorage(const uint16_t size, const std::string& name) :
    MappedStorage<MemoryAddr, Instruction>(size, name),
    readPointer(size),
    writePointer(size),
    locations(size) {

  readPointer.setNull();
  writePointer = -1;

  jumpAmount = 0;

  fillCount = 0;
  currentPacket.reset();
  pendingPacket.reset();

  currentPacket.execute = true;
  finishedPacket = false;

  // Set all tags to a default value which is unlikely to be encountered in
  // execution.
  tags.assign(tags.size(), DEFAULT_TAG);
}
