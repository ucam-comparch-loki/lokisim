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
bool IPKCacheStorage::checkTags(const MemoryAddr& key, operation_t operation) {
  // Depending on whether are are currently executing a packet or not, we may
  // want to store information in either currentPacket or pendingPacket.
  PacketInfo* packet;
  bool foundMatch = false;

  for(uint i=0; i<this->tags.size(); i++) {
    if(this->tags[i] == key) {  // Found a match

      // If we don't have an instruction ready to execute next, jump to the one
      // we just found.
      if(readPointer.isNull()) {
        readPointer = i;
        packet = &currentPacket;

        updateFillCount();
      }
      // If we do have an instruction ready to execute next, queue the packet
      // we just found to execute when this packet finishes.
      else {
        packet = &pendingPacket;
      }

      packet->cacheIndex = i;

      foundMatch = true;
      break;
    }
  }

  if(!foundMatch) {
    if(readPointer.isNull()) packet = &currentPacket;
    else                     packet = &pendingPacket;
    packet->cacheIndex = NOT_IN_CACHE;
  }

  packet->memAddr    = key;
  packet->inCache    = foundMatch;
  packet->isFill     = operation == InstructionMap::FILL;     // TODO: FILLR
  packet->persistent = operation == InstructionMap::FETCHPST; // TODO: FETCHPSTR

  return foundMatch;

}

/* Returns the next item in the cache. */
const Instruction& IPKCacheStorage::read() {
  assert(!readPointer.isNull());

  int i = readPointer.value();
  incrementReadPos();

  previousLocation = locations[i];

  return this->data_[i];
}

/* Writes new data to a position determined using the given key.
 * An exception is thrown if the packet queued up to execute next is being
 * overwritten. */
void IPKCacheStorage::write(const MemoryAddr& key, const Instruction& newData) {
  this->tags[writePointer.value()] = key;
  this->data_[writePointer.value()] = newData;

  // Determine whether this instruction is part of the packet which is currently
  // executing, or part of the packet due to execute next.
  PacketInfo* packet;
  if(!currentPacket.inCache) packet = &currentPacket;
  else                       packet = &pendingPacket;

  // If this is the first instruction of the packet, keep track of where in the
  // cache it is going (allows us to jump there when this packet finishes).
  if(packet->cacheIndex == NOT_IN_CACHE) packet->cacheIndex = writePointer.value();

  // Store memory address of this instruction for debug reasons.
  if(key == DEFAULT_TAG) {
    // If there is no supplied address, this is the continuation of an
    // instruction packet, and so this instruction is next to the previous
    // one.
    MemoryAddr prev = this->locations[writePointer-1];
    MemoryAddr newAddr = prev + BYTES_PER_INSTRUCTION;
    locations[writePointer.value()] = newAddr;
  }
  else locations[writePointer.value()] = key;

  // If we're not serving instructions at the moment, start serving from here.
  if(readPointer.isNull()) readPointer = writePointer;

  // If this is the end of an instruction packet, mark the packet as being
  // in the cache - this may allow the next fetch request to be sent.
  if(newData.endOfPacket()) packet->inCache = true;

  incrementWritePos();
}

/* Jump to a new instruction at a given offset. */
void IPKCacheStorage::jump(const JumpOffset offset) {

  // Hack: if we move to the next packet, then discover we should have jumped,
  // reset the next packet pointer.
  if(this->tags[readPointer.value()] != DEFAULT_TAG && !empty()) {
    pendingPacket.cacheIndex = readPointer.value();
  }

  // Restore the read pointer if it was not being used.
  if(readPointer.isNull()) readPointer = currInstBackup + offset;
  else readPointer += offset - 1; // -1 because we have already incremented readPointer

  updateFillCount();

  // Update currentPacket if we have jumped to the start of a packet, or if
  // currentPacket was previously an invalid value.
  if(this->tags[readPointer.value()] != DEFAULT_TAG || currentPacket.cacheIndex == NOT_IN_CACHE) {
    currentPacket.cacheIndex = readPointer.value();
  }

  if(DEBUG) cout << "Jumped by " << (int)offset << " to instruction " <<
      readPointer.value() << endl;
}

/* Return the memory address of the currently-executing packet. Only returns
 * a useful value for the first instruction in each packet, as this is the
 * only instruction which gets a tag. */
const MemoryAddr IPKCacheStorage::packetAddress() const {
  return this->tags[currentPacket.cacheIndex];
}

/* Returns the remaining number of entries in the cache. */
const uint16_t IPKCacheStorage::remainingSpace() const {
  return this->size() - fillCount;
}

const MemoryAddr IPKCacheStorage::getInstLocation() const {
  return previousLocation;
}

/* Returns whether the cache is empty. Note that even if a cache is empty,
 * it is still possible to access its contents if an appropriate tag is
 * looked up.
 * The cache pretends to be empty if the current packet is being filled (and
 * not fetched). This is because we don't want the packet to execute yet. */
bool IPKCacheStorage::empty() const {
  return (readPointer.isNull()) || (fillCount == 0) || currentPacket.isFill;
}

/* Returns whether the cache is full. */
bool IPKCacheStorage::full() const {
  return fillCount == this->size();
}

/* We can issue a new fetch command if there is space in the cache for a
 * maximum-sized instruction packet, and no other fetches are already in
 * progress. */
bool IPKCacheStorage::canFetch() const {
  return (remainingSpace() >= MAX_IPK_SIZE) &&
      !currentPacket.arriving() && !pendingPacket.arriving();
}

/* Begin reading the packet which is queued up to execute next. */
void IPKCacheStorage::switchToPendingPacket() {
  currInstBackup = readPointer - 1;  // We have incremented readPointer

  if(currentPacket.persistent)
    readPointer = currentPacket.cacheIndex;
  else {
    currentPacket = pendingPacket;
    pendingPacket.reset();

    if(currentPacket.cacheIndex == NOT_IN_CACHE)
      readPointer.setNull();
    else
      readPointer = currentPacket.cacheIndex;
  }

  updateFillCount();

  if(DEBUG) cout << "Switched to pending packet: current = " <<
      readPointer.value() << ", refill = " << writePointer.value() << endl;
}

/* Store some initial instructions in the cache. */
void IPKCacheStorage::storeCode(const std::vector<Instruction>& code) {
  if(code.size() > this->size()) {
    cerr << "Error: tried to write " << code.size() <<
      " instructions to a instruction cache of size " << this->size() << endl;
  }

  for(uint i=0; i<code.size() && i<this->size(); i++) {
    write(DEFAULT_TAG, code[i]);
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

void IPKCacheStorage::incrementWritePos() {
  ++writePointer;
  fillCount++;
}

void IPKCacheStorage::incrementReadPos() {
  ++readPointer;
  fillCount--;
}

void IPKCacheStorage::updateFillCount() {
  if(writePointer == readPointer)
    fillCount = this->size();
  else if(readPointer.isNull())
    fillCount = 0;
  else
    fillCount = writePointer - readPointer;
}

IPKCacheStorage::IPKCacheStorage(const uint16_t size, const std::string& name) :
    MappedStorage<MemoryAddr, Instruction>(size, name),
    readPointer(size),
    writePointer(size),
    locations(size) {

  readPointer.setNull();
  writePointer = 0;

  fillCount = 0;
  currentPacket.reset();
  pendingPacket.reset();

  // Set all tags to a default value which is unlikely to be encountered in
  // execution.
  tags.assign(tags.size(), DEFAULT_TAG);
}
