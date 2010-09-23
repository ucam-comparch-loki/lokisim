/*
 * IPKCacheStorage.cpp
 *
 *  Created on: 14 Jun 2010
 *      Author: db434
 */

#include "IPKCacheStorage.h"
#include "../Datatype/Address.h"
#include "../Datatype/Instruction.h"
#include "../Exceptions/ReadingFromEmptyException.h"
#include "../Utility/Parameters.h"

/* Returns whether the given address matches any of the tags. */
bool IPKCacheStorage::checkTags(const Address& key) {

  for(uint i=0; i<this->size(); i++) {
    if(this->tags[i] == key) {
      if(currInst.isNull()) {
        currInst = i;
        currentPacket = i;
      }
      else pendingPacket = i;

      return true;
    }
  }

  // If we have escaped the loop, the tag is not in the cache.

  // The next packet to execute will be the one which is about to be fetched.
  pendingPacket = NOT_IN_USE;
  return false;

}

/* Returns the next item in the cache. */
Instruction& IPKCacheStorage::read() {

  if(!currInst.isNull()) {
    int i = currInst.value();
    incrementCurrent();
    lastOpWasARead = true;

    return this->data[i];
  }
  else {
    throw ReadingFromEmptyException("instruction packet cache");
  }

}

/* Writes new data to a position determined using the given key. */
void IPKCacheStorage::write(const Address& key, const Instruction& newData) {
  this->tags[refill.value()] = key;
  this->data[refill.value()] = newData;

  bool needRefetch = false;

  // If we're not serving instructions at the moment, start serving from here.
  if(currInst.isNull()) {
    currInst = refill.value();
    currentPacket = refill.value();

    // If we're executing this packet repeatedly, set it to execute next too.
    if(persistentMode) pendingPacket = refill.value();
  }
  // If it's the start of a new packet, queue it up to execute next.
  // A default key value shows that the instruction is continuing a packet.
  else if(!(key == Address())) pendingPacket = refill.value();
  // We need to fetch the pending packet if we are now overwriting it.
  else needRefetch = (refill == pendingPacket);

  lastOpWasARead = false;

  incrementRefill();

  if(needRefetch) {
    pendingPacket = NOT_IN_USE;
    throw std::exception(); // Use a subclass of exception?
  }

}

/* Jump to a new instruction at a given offset. */
void IPKCacheStorage::jump(const int16_t offset) {

  if(currInst.isNull()) currInst = currInstBackup-1;

  currInst += offset - 1; // -1 because we have already incremented currInst
  updateFillCount();

  // Update currentPacket if we have jumped to the start of a packet
  if(!(this->tags[currInst.value()] == Address())) {
    currentPacket = currInst.value();
  }

  if(DEBUG) cout << "Jumped by " << (int)offset << " to instruction " <<
      currInst.value() << endl;
}

/* Return the memory address of the currently-executing packet. */
Address IPKCacheStorage::packetAddress() const {
  return this->tags[currentPacket];
}

/* Returns the remaining number of entries in the cache. */
uint16_t IPKCacheStorage::remainingSpace() const {
  uint16_t space = this->size() - fillCount;
  return space;
}

uint16_t IPKCacheStorage::getInstIndex() const {
//  Address startOfPacket = packetAddress();
//  int cacheIndex = (currInst==NOT_IN_USE) ? currInstBackup : currInst.value();
//  int packetIndex = cacheIndex - 1 - currentPacket;
//  if(packetIndex<0) packetIndex += IPK_CACHE_SIZE;
//
//  cout << cacheIndex << " " << packetIndex << " " << endl;
//
//  return startOfPacket.getAddress() + BYTES_PER_WORD*packetIndex;

  return -1;
}

/* Returns whether the cache is empty. Note that even if a cache is empty,
 * it is still possible to access its contents if an appropriate tag is
 * looked up. */
bool IPKCacheStorage::isEmpty() const {
  // Definition of "empty" is slightly tricky: if the current instruction
  // pointer and the refill pointer are in the same place, this could mean
  // that the cache is either empty or full. Need to take into account
  // whether the last operation was a read or a write.
  // If it was a read, the cache is now empty; if it was a write, it is full.
  return (currInst.isNull()) || (fillCount == 0 && lastOpWasARead);
}

/* Returns whether the cache is full. */
bool IPKCacheStorage::isFull() const {
  return fillCount == this->size();
}

/* Begin reading the packet which is queued up to execute next. */
void IPKCacheStorage::switchToPendingPacket() {

  currInstBackup = currInst.value();// - 1;  // We have incremented currInst
  if(pendingPacket == NOT_IN_USE) currInst.setNull();
  else                            currInst = pendingPacket;
  currentPacket = pendingPacket;

  if(!persistentMode) pendingPacket = NOT_IN_USE;
  updateFillCount();

  if(DEBUG) cout << "Switched to pending packet: current = " <<
      currInst.value() << ", refill = " << refill.value() << endl;
}

void IPKCacheStorage::setPersistent(const bool persistent) {
  persistentMode = persistent;
}

/* Store some initial instructions in the cache. */
void IPKCacheStorage::storeCode(const std::vector<Instruction>& code) {
  if(code.size() > this->size()) {
    cerr << "Error: tried to write " << code.size() <<
      " instructions to a memory of size " << this->size() << endl;
  }

  for(uint i=0; i<code.size() && i<this->size(); i++) {
    write(Address(), code[i]);
  }
}

/* Returns the data corresponding to the given address.
 * Private because we don't want to use this version for IPK caches. */
Instruction& IPKCacheStorage::read(const Address& key) {
  throw new std::exception();
}

/* Returns the position that data with the given address tag should be stored. */
uint16_t IPKCacheStorage::getPosition(const Address& key) {
  return refill.value();
}

void IPKCacheStorage::incrementRefill() {
  ++refill;
  updateFillCount();
}

void IPKCacheStorage::incrementCurrent() {
  ++currInst;

  // We don't use updateFillCount() since we want the possibility of the
  // cache being empty. updateFillCount() always assumes fullness.
  fillCount = (fillCount - 1 + this->size()) % this->size();
}

void IPKCacheStorage::updateFillCount() {
  if(refill == currInst) {
    fillCount = this->size();
    return;
  }

  // Add the size of the cache to ensure that the value is not negative.
  fillCount = (refill - currInst + this->size()) % this->size();
}

IPKCacheStorage::IPKCacheStorage(const uint16_t size) :
    MappedStorage<Address, Instruction>(size),
    currInst(size),
    refill(size) {

  currInst.setNull();
  refill = 0;

  fillCount = 0;
  pendingPacket = NOT_IN_USE;

  lastOpWasARead = true;

}

IPKCacheStorage::~IPKCacheStorage() {

}
