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

#define NOT_IN_USE -1

template<class K, class T>
class IPKCacheStorage : public MappedStorage<K,T> {

public:

/* Constructors and destructors */
  IPKCacheStorage(int size) : MappedStorage<K,T>(size) {
    currentInstruction = NOT_IN_USE;
    refillPointer = 0;
    fillCount = 0;
    pendingPacket = NOT_IN_USE;
  }

  virtual ~IPKCacheStorage() {

  }

/* Methods */
  // Returns whether the given address matches any of the tags
  virtual bool checkTags(const K& key) {

    for(unsigned int i=0; i<MappedStorage<K,T>::tags.size(); i++) {
      if(MappedStorage<K,T>::tags[i] == key) {
        if(currentInstruction == NOT_IN_USE) currentInstruction = i;
        else pendingPacket = i;

        return true;
      }
    }
    return false;

  }

  // Returns the next item in the cache
  virtual T& read() {

    if((int)currentInstruction != NOT_IN_USE) {
      int i = currentInstruction;
      incrementCurrent();
      return Storage<T>::data[i];
    }
    else {
      std::cerr << "Exception in IPKCacheStorage.read(): cache is empty."
                << std::endl;
      throw(std::exception());
    }

  }

  // Writes new data to a position determined using the given key
  virtual void write(const K& key, const T& newData) {
    MappedStorage<K,T>::tags[refillPointer] = key;
    Storage<T>::data[refillPointer] = newData;

    if(currentInstruction == NOT_IN_USE) currentInstruction = refillPointer;

    incrementRefill();
  }

  int remainingSpace() const {
    int space = Storage<T>::data.size() - fillCount;
    return space;
  }

  bool isEmpty() const {
    return fillCount == 0;
  }

  bool isFull() const {
    return fillCount == (int)Storage<T>::data.size();
  }

  void switchToPendingPacket() {
    currentInstruction = pendingPacket;
    pendingPacket = NOT_IN_USE;
  }

  void storeCode(std::vector<T>& code) {
    if(code.size() > Storage<T>::data.size()) {
      std::cerr << "Error: tried to write " << code.size() <<
        " instructions to a memory of size " << Storage<T>::data.size() <<
        std::endl;
    }

    for(unsigned int i=0; i<code.size() && i<Storage<T>::data.size(); i++) {
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
    return refillPointer;
  }

  void incrementRefill() {
    if(refillPointer >= (int)Storage<T>::data.size()-1) refillPointer = 0;
    else refillPointer++;

    fillCount++;
  }

  void incrementCurrent() {
    if(currentInstruction >= (int)Storage<T>::data.size()-1) currentInstruction = 0;
    else currentInstruction++;

    fillCount--;
  }

/* Local state */
  int currentInstruction, refillPointer, fillCount;

  // Do we want a single pending packet, or a queue of them?
  int pendingPacket;  // Location of the next packet to be executed

};

#endif /* IPKCACHESTORAGE_H_ */
