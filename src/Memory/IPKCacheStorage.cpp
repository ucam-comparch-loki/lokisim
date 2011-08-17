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

const MemoryAddr IPKCacheStorage::DEFAULT_TAG;

/* Returns whether the given address matches any of the tags. */
bool IPKCacheStorage::checkTags(const MemoryAddr& key) {

  for(uint i=0; i<this->tags.size(); i++) {
    if(this->tags[i] == key) {  // Found a match

      // If we don't have an instruction ready to execute next, jump to the one
      // we just found.
      if(readPointer.isNull()) {
        readPointer = i;
        currentPacket = i;

        updateFillCount();
      }
      // If we do have an instruction ready to execute next, queue the packet
      // we just found to execute when this packet finishes.
      else {
        pendingPacket = i;
      }

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
  if(readPointer.isNull()) {
    readPointer = writePointer;
    currentPacket = writePointer.value();

    // If we're executing this packet repeatedly, set it to execute next too.
    if(persistentMode) pendingPacket = writePointer.value();
  }
  // If it's the start of a new packet, queue it up to execute next.
  // A default key value shows that the instruction is continuing a packet.
  else if(key != DEFAULT_TAG) {
    pendingPacket = writePointer.value();
  }
  // We need to refetch the pending packet if we are now overwriting it.
  else if(writePointer == pendingPacket) {
    incrementWritePos();

    pendingPacket = NOT_IN_USE;
    throw std::exception(); // Use a subclass of exception?
  }

  incrementWritePos();
}

/* Jump to a new instruction at a given offset. */
void IPKCacheStorage::jump(const JumpOffset offset) {

  // Hack: if we move to the next packet, then discover we should have jumped,
  // reset the next packet pointer.
  if(this->tags[readPointer.value()] != DEFAULT_TAG && !empty()) {
    pendingPacket = readPointer.value();
  }

  // Restore the read pointer if it was not being used.
  if(readPointer.isNull()) readPointer = currInstBackup + offset;
  else readPointer += offset - 1; // -1 because we have already incremented readPointer

  updateFillCount();

  // Update currentPacket if we have jumped to the start of a packet, or if
  // currentPacket was previously an invalid value.
  if(this->tags[readPointer.value()] != DEFAULT_TAG || currentPacket == NOT_IN_USE) {
    currentPacket = readPointer.value();
  }

  if(DEBUG) cout << "Jumped by " << (int)offset << " to instruction " <<
      readPointer.value() << endl;
}

/* Return the memory address of the currently-executing packet. Only returns
 * a useful value for the first instruction in each packet, as this is the
 * only instruction which gets a tag. */
const MemoryAddr IPKCacheStorage::packetAddress() const {
  return this->tags[currentPacket];
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
 * looked up. */
bool IPKCacheStorage::empty() const {
  return (readPointer.isNull()) || (fillCount == 0);
}

/* Returns whether the cache is full. */
bool IPKCacheStorage::full() const {
  return fillCount == this->size();
}

/* Begin reading the packet which is queued up to execute next. */
void IPKCacheStorage::switchToPendingPacket() {
  currInstBackup = readPointer - 1;  // We have incremented readPointer

  if(pendingPacket == NOT_IN_USE) readPointer.setNull();
  else                            readPointer = pendingPacket;

  currentPacket = pendingPacket;
  if(!persistentMode) pendingPacket = NOT_IN_USE;

  updateFillCount();

  if(DEBUG) cout << "Switched to pending packet: current = " <<
      readPointer.value() << ", refill = " << writePointer.value() << endl;
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
  pendingPacket = NOT_IN_USE;

  // Set all tags to a default value which is unlikely to be encountered in
  // execution.
  tags.assign(tags.size(), DEFAULT_TAG);
}
