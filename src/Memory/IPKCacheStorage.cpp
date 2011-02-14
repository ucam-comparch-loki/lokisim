/*
 * IPKCacheStorage.cpp
 *
 *  Created on: 14 Jun 2010
 *      Author: db434
 */

#include "IPKCacheStorage.h"
#include "../Datatype/Instruction.h"
#include "../Exceptions/ReadingFromEmptyException.h"
#include "../Utility/Parameters.h"

/* Returns whether the given address matches any of the tags. */
bool IPKCacheStorage::checkTags(const Address& key) {

  for(uint i=0; i<this->tags.size(); i++) {
    if(this->tags[i] == key) {
      if(currInst.isNull()) {
        currInst = i;
        currentPacket = i;

        // If we've found a packet in the cache, we know it is valid and
        // will want to read it.
        lastOpWasARead = false;
        updateFillCount();
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
const Instruction& IPKCacheStorage::read() {

  if(!currInst.isNull()) {
    int i = currInst.value();
    incrementCurrent();

    lastOpWasARead = true;

    previousLocation = locations[i];
    return this->data_[i];
  }
  else {
    throw ReadingFromEmptyException("instruction packet cache");
  }

}

/* Writes new data to a position determined using the given key. */
void IPKCacheStorage::write(const Address& key, const Instruction& newData) {
  this->tags[refill.value()] = key;
  this->data_[refill.value()] = newData;

  // Store memory address of this instruction for debug reasons.
  if(key == Address()) {
    // If there is no supplied address, this is the continuation of an
    // instruction packet, and so this instruction is next to the previous
    // one.
    Address prev = this->locations[refill-1];
    Address newAddr = prev.addOffset(BYTES_PER_INSTRUCTION);
    locations[refill.value()] = newAddr;
  }
  else locations[refill.value()] = key;

  bool needRefetch = false;

  // If we're not serving instructions at the moment, start serving from here.
  if(currInst.isNull()) {
    currInst = refill;
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
void IPKCacheStorage::jump(const JumpOffset offset) {

  // Hack: if we move to the next packet, then discover we should have jumped,
  // reset the next packet pointer.
  if(!(this->tags[currInst.value()] == Address())) {
    pendingPacket = currInst.value();
  }

  // Restore the current instruction pointer if it was not being used.
  if(currInst.isNull()) currInst = currInstBackup + offset;
  else currInst += offset - 1; // -1 because we have already incremented currInst

  // We have just jumped, so expect there to be data to read. Ensure that in
  // the case where refill = current, we don't see the cache as being empty.
  lastOpWasARead = false;
  updateFillCount();

  // Update currentPacket if we have jumped to the start of a packet, or if
  // currentPacket was previously an invalid value.
  if(!(this->tags[currInst.value()] == Address()) || currentPacket == NOT_IN_USE) {
    currentPacket = currInst.value();
  }

  if(DEBUG) cout << "Jumped by " << (int)offset << " to instruction " <<
      currInst.value() << endl;
}

/* Return the memory address of the currently-executing packet. Only returns
 * a useful value for the first instruction in each packet, as this is the
 * only instruction which gets a tag. */
const Address IPKCacheStorage::packetAddress() const {
  return this->tags[currentPacket];
}

/* Returns the remaining number of entries in the cache. */
const uint16_t IPKCacheStorage::remainingSpace() const {
  return this->size() - fillCount;
}

const Address IPKCacheStorage::getInstLocation() const {
  return previousLocation;
}

/* Returns whether the cache is empty. Note that even if a cache is empty,
 * it is still possible to access its contents if an appropriate tag is
 * looked up. */
bool IPKCacheStorage::empty() const {
  // Definition of "empty" is slightly tricky: if the current instruction
  // pointer and the refill pointer are in the same place, this could mean
  // that the cache is either empty or full. Need to take into account
  // whether the last operation was a read or a write.
  // If it was a read, the cache is now empty; if it was a write, it is full.
  return (currInst.isNull()) || (fillCount == 0);
}

/* Returns whether the cache is full. */
bool IPKCacheStorage::full() const {
  return fillCount == this->size();
}

/* Begin reading the packet which is queued up to execute next. */
void IPKCacheStorage::switchToPendingPacket() {
  currInstBackup = currInst - 1;  // We have incremented currInst

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
      " instructions to a instruction cache of size " << this->size() << endl;
  }

  for(uint i=0; i<code.size() && i<this->size(); i++) {
    write(Address(), code[i]);
  }
}

/* Returns the data corresponding to the given address.
 * Private because we don't want to use this version for IPK caches. */
const Instruction& IPKCacheStorage::read(const Address& key) {
  throw new std::exception();
}

/* Returns the position that data with the given address tag should be stored. */
uint16_t IPKCacheStorage::getPosition(const Address& key) {
  return refill.value();
}

void IPKCacheStorage::incrementRefill() {
  ++refill;
  fillCount++;
}

void IPKCacheStorage::incrementCurrent() {
  ++currInst;
  fillCount--;
}

void IPKCacheStorage::updateFillCount() {
  if(refill == currInst) {
    // If we just read something, the cache must now be empty.
    // If we wrote something, it must now be full.
    fillCount = lastOpWasARead ? 0 : this->size();
    return;
  }
  else if(currInst.isNull()) {
    fillCount = 0;
    return;
  }

  // Add the size of the cache to ensure that the value is not negative.
  fillCount = (refill - currInst + this->size()) % this->size();
}

IPKCacheStorage::IPKCacheStorage(const uint16_t size, std::string name) :
    MappedStorage<Address, Instruction>(size, name),
    currInst(size),
    refill(size),
    locations(size) {

  currInst.setNull();
  refill = 0;

  fillCount = 0;
  pendingPacket = NOT_IN_USE;

  lastOpWasARead = true;

}

IPKCacheStorage::~IPKCacheStorage() {

}
