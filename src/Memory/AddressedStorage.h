/*
 * AddressedStorage.h
 *
 * Represents the class of storage objects where data is accessed using its
 * address in the structure, rather than its address in main memory, or some
 * other tag.
 *
 * Since this class is templated, all of the implementation must go in the
 * header file.
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef ADDRESSEDSTORAGE_H_
#define ADDRESSEDSTORAGE_H_

#include "Storage.h"

template<class T>
class AddressedStorage : public Storage<T> {

//==============================//
// Methods
//==============================//

public:

  // Read from the given address in the memory.
  virtual const T& read(int addr) const {
    this->checkBounds(addr);
    // Templated inheritance hides all inherited names - need to access them
    // like this.
    return this->data[addr];
  }

  // Write the given data to the given address in memory.
  virtual void write(T& newData, int addr) {
    this->checkBounds(addr);
    this->data[addr] = newData;
  }

//==============================//
// Constructors and destructors
//==============================//

public:

  AddressedStorage(int size) : Storage<T>(size) {

  }

  virtual ~AddressedStorage() {

  }

};

#endif /* ADDRESSEDSTORAGE_H_ */
