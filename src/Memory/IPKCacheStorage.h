/*
 * IPKCacheStorage.h
 *
 * A special version of the MappedStorage class with behaviour tailored to
 * the Instruction Packet Cache.
 *
 * Since this class is templated, all of the implementation must go in the
 * header file.
 *
 *  Created on: 26 Jan 2010
 *      Author: db434
 */

#ifndef IPKCACHESTORAGE_H_
#define IPKCACHESTORAGE_H_

#include "MappedStorage.h"
#include "../Utility/LoopCounter.h"

template<class K, class T>
class IPKCacheStorage : public MappedStorage<K,T> {

//==============================//
// Constructors and destructors
//==============================//

public:

  IPKCacheStorage(short size) :
      MappedStorage<K,T>(size),
      currInst(size),
      refill(size) {

    currInst = NOT_IN_USE;
    refill = 0;

    fillCount = 0;
    pendingPacket = NOT_IN_USE;

  }

  virtual ~IPKCacheStorage() {

  }

//==============================//
// Methods
//==============================//

public:

  // Returns whether the given address matches any of the tags
  virtual bool checkTags(const K& key) {

    for(int i=0; i<this->size(); i++) {
      if(this->tags[i] == key) {
        if(currInst == NOT_IN_USE) currInst = i;
        else pendingPacket = i;

        return true;
      }
    }

    // If we have escaped the loop, the tag is not in the cache.

    // The next packet to execute will be the one which is about to be fetched.
    pendingPacket = NOT_IN_USE;
    return false;

  }

  // Returns the next item in the cache
  virtual T& read() {

    if(currInst != NOT_IN_USE) {
      int i = currInst.value();
      incrementCurrent();
      return this->data[i];
    }
    else {
      cerr << "Exception in IPKCacheStorage.read(): cache is empty." << endl;
      throw(std::exception());
    }

  }

  // Writes new data to a position determined using the given key
  virtual void write(const K& key, const T& newData) {
    this->tags[refill.value()] = key;
    this->data[refill.value()] = newData;

    bool needRefetch = false;

    // If we're not serving instructions at the moment, start serving from here.
    if(currInst == NOT_IN_USE) currInst = refill.value();
    // If it's the start of a new packet, queue it up to execute next.
    // A default key value shows that the instruction is continuing a packet.
    else if(!(key == K())) pendingPacket = refill.value();
    // We need to fetch the pending packet if we are now overwriting it.
    else needRefetch = (refill == pendingPacket);

    incrementRefill();

    if(needRefetch) {
      pendingPacket = NOT_IN_USE;
      throw std::exception(); // Use a subclass of exception?
    }

  }

  // Jump to a new instruction at a given offset.
  void jump(int offset) {
    if(currInst == NOT_IN_USE) currInst = currInstBackup;

    currInst += offset - 1;
    updateFillCount();

    if(DEBUG) cout << "Jumped by " << offset << " to instruction " <<
        currInst.value() << endl;
  }

  // Return the memory address of the currently-executing packet.
  K& packetAddress() {
//    if(currInst == NOT_IN_USE) return K();//throw std::exception();
    return this->tags[currInst-1];
  }

  // Returns the remaining number of entries in the cache.
  int remainingSpace() const {
    int space = this->size() - fillCount;
    return space;
  }

  // Returns whether the cache is empty. Note that even if a cache is empty,
  // it is still possible to access its contents if an appropriate tag is
  // looked up.
  bool isEmpty() const {
    return (currInst == NOT_IN_USE);// || (fillCount == 0);
  }

  // Returns whether the cache is full.
  bool isFull() const {
    return fillCount == this->size();
  }

  // Begin reading the packet which is queued up to execute next.
  void switchToPendingPacket() {
    currInstBackup = currInst - 1;
    currInst = pendingPacket;
    pendingPacket = NOT_IN_USE;
    updateFillCount();
    cout << "Switched to pending packet: current = " << currInst.value() << ", refill = " << refill.value() << endl;
  }

  // Store some initial instructions in the cache.
  void storeCode(std::vector<T>& code) {
    if((int)code.size() > this->size()) {
      cerr << "Error: tried to write " << code.size() <<
        " instructions to a memory of size " << this->size() << endl;
    }

    for(int i=0; i<(int)code.size() && i<this->size(); i++) {
      write(K(), code[i]);
    }
  }

private:

  // Returns the data corresponding to the given address.
  // Private because we don't want to use this version for IPK caches.
  virtual T& read(const K& key) {
    throw new std::exception();
  }

  // Returns the position that data with the given address tag should be stored
  virtual int getPosition(const K& key) {
    return refill.value();
  }

  void incrementRefill() {
    ++refill;
    updateFillCount();
  }

  void incrementCurrent() {
    ++currInst;

    // We don't use updateFillCount() since we want the possibility of the
    // cache being empty. updateFillCount() always assumes fullness.
    fillCount = (fillCount - 1 + this->size()) % this->size();
  }

  void updateFillCount() {
    if(refill == currInst) {
      fillCount = this->size();
      return;
    }

    // Add the size of the cache to ensure that the value is not negative.
    fillCount = (refill - currInst + this->size()) % this->size();
  }

//==============================//
// Local state
//==============================//

private:

  LoopCounter currInst, refill;
  int fillCount;
  int currInstBackup;   // In case it goes NOT_IN_USE and then a jump is used

  // Do we want a single pending packet, or a queue of them?
  int pendingPacket;  // Location of the next packet to be executed

  static const int NOT_IN_USE = -1;

};

#endif /* IPKCACHESTORAGE_H_ */
