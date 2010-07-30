/*
 * IPKCacheStorage.cpp
 *
 *  Created on: 14 Jun 2010
 *      Author: db434
 */

#include "IPKCacheStorage.h"
#include "../Utility/Parameters.h"

/* Returns whether the given address matches any of the tags. */
bool IPKCacheStorage::checkTags(const Address& key) {

  for(int i=0; i<this->size(); i++) {
    if(this->tags[i] == key) {
      if(currInst == NOT_IN_USE) {
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

  if(currInst != NOT_IN_USE) {
    int i = currInst.value();
    incrementCurrent();
    lastOpWasARead = true;
    return this->data[i];
  }
  else {
    cerr << "Exception in IPKCacheStorage.read(): cache is empty." << endl;
    throw(std::exception());
  }

}

/* Writes new data to a position determined using the given key. */
void IPKCacheStorage::write(const Address& key, const Instruction& newData) {
  this->tags[refill.value()] = key;
  this->data[refill.value()] = newData;

  bool needRefetch = false;

  // If we're not serving instructions at the moment, start serving from here.
  if(currInst == NOT_IN_USE) {
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
void IPKCacheStorage::jump(int offset) {

  if(currInst == NOT_IN_USE) currInst = currInstBackup;

  currInst += offset - 2; // -1 because we have already incremented currInst
  updateFillCount();

  // Update currentPacket if we have jumped to the start of a packet
  if(!(this->tags[currInst.value()] == Address()))
    currentPacket = currInst.value();

  if(DEBUG) cout << "Jumped by " << offset << " to instruction " <<
      currInst.value() << endl;
}

/* Return the memory address of the currently-executing packet. */
Address& IPKCacheStorage::packetAddress() {
  return this->tags[currentPacket];
}

/* Returns the remaining number of entries in the cache. */
int IPKCacheStorage::remainingSpace() const {
  int space = this->size() - fillCount;
  return space;
}

/* Returns whether the cache is empty. Note that even if a cache is empty,
 * it is still possible to access its contents if an appropriate tag is
 * looked up. */
bool IPKCacheStorage::isEmpty() const {
  // Definition of "empty" is slightly tricky: if the current instruction
  // pointer and the refill pointer are in the same place, this could mean
  // that the cache is either empty or full. Need to take into account
  // whether the last operation was a read or a write.
  return (currInst == NOT_IN_USE) || (fillCount == 0 && lastOpWasARead);
}

/* Returns whether the cache is full. */
bool IPKCacheStorage::isFull() const {
  return fillCount == this->size();
}

/* Begin reading the packet which is queued up to execute next. */
void IPKCacheStorage::switchToPendingPacket() {

  currInstBackup = currInst.value();// - 1;  // We have incremented currInst
  currInst = pendingPacket;
  currentPacket = pendingPacket;

  if(!persistentMode) pendingPacket = NOT_IN_USE;
  updateFillCount();

  if(DEBUG) cout << "Switched to pending packet: current = " <<
      currInst.value() << ", refill = " << refill.value() << endl;
}

void IPKCacheStorage::setPersistent(bool persistent) {
  persistentMode = persistent;
}

/* Store some initial instructions in the cache. */
void IPKCacheStorage::storeCode(std::vector<Instruction>& code) {
  if((int)code.size() > this->size()) {
    cerr << "Error: tried to write " << code.size() <<
      " instructions to a memory of size " << this->size() << endl;
  }

  for(int i=0; i<(int)code.size() && i<this->size(); i++) {
    write(Address(), code[i]);
  }
}

/* Returns the data corresponding to the given address.
 * Private because we don't want to use this version for IPK caches. */
Instruction& IPKCacheStorage::read(const Address& key) {
  throw new std::exception();
}

/* Returns the position that data with the given address tag should be stored. */
int IPKCacheStorage::getPosition(const Address& key) {
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

IPKCacheStorage::IPKCacheStorage(short size) :
    MappedStorage<Address, Instruction>(size),
    currInst(size),
    refill(size) {

  currInst = NOT_IN_USE;
  refill = 0;

  fillCount = 0;
  pendingPacket = NOT_IN_USE;

  lastOpWasARead = true;

}

IPKCacheStorage::~IPKCacheStorage() {

}
