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

template<class K, class T>
class IPKCacheStorage : public MappedStorage<K,T> {

//==============================//
// Constructors and destructors
//==============================//

public:

  IPKCacheStorage(short size) : MappedStorage<K,T>(size) {
    currentInstruction = NOT_IN_USE;
    refillPointer = 0;
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

  // Jump to a new instruction at a given offset.
  void jump(int offset) {
    if(currentInstruction == NOT_IN_USE) currentInstruction = currInstBackup;

    currentInstruction += offset - 1; // -1 because we've incremented

    // Bring it back within required bounds
    if(currentInstruction >= (int)Storage<T>::data.size()) {
      currentInstruction -= Storage<T>::data.size();
    }
    else if(currentInstruction < 0) {
      currentInstruction += Storage<T>::data.size();
    }

    updateFillCount();

    if(DEBUG) cout << "Jumped by " << offset << " to instruction " <<
        currentInstruction << endl;
  }

  int remainingSpace() const {
    int space = Storage<T>::data.size() - fillCount;
    return space;
  }

  bool isEmpty() const {
    return (currentInstruction == NOT_IN_USE) || (fillCount == 0);
  }

  bool isFull() const {
    return fillCount == (int)Storage<T>::data.size();
  }

  void switchToPendingPacket() {
    currInstBackup = currentInstruction - 1;
    currentInstruction = pendingPacket;
    pendingPacket = NOT_IN_USE;
  }

  void storeCode(std::vector<T>& code) {
    if(code.size() > Storage<T>::data.size()) {
      std::cerr << "Error: tried to write " << code.size() <<
        " instructions to a memory of size " << Storage<T>::data.size() << endl;
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

  void updateFillCount() {
    if(currentInstruction <= refillPointer) {
      fillCount = refillPointer - currentInstruction;
    }
    else {
      fillCount = refillPointer - currentInstruction + Storage<T>::data.size();
    }
  }

//==============================//
// Local state
//==============================//

private:

  int currentInstruction, refillPointer, fillCount;
  int currInstBackup;   // In case it goes NOT_IN_USE and then a jump is used

  // Do we want a single pending packet, or a queue of them?
  int pendingPacket;  // Location of the next packet to be executed

  static const int NOT_IN_USE = -1;

};

#endif /* IPKCACHESTORAGE_H_ */
