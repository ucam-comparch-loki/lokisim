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

//============================================================================//
// Methods
//============================================================================//

public:

  // Read from the given address in the memory.
  virtual const T& read(const uint32_t addr) const {
    this->checkBounds(addr);
    return this->data_[addr];
  }

  // Write the given data to the given address in memory.
  virtual void write(const T& newData, const uint32_t addr) {
    this->checkBounds(addr);
    this->data_[addr] = newData;
  }

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  AddressedStorage(const std::string& name, const uint32_t size) :
    Storage<T>(name, size) {

  }

  virtual ~AddressedStorage() {}

};

#endif /* ADDRESSEDSTORAGE_H_ */
