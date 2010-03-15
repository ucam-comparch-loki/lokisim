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

public:
/* Methods */
  virtual const T& read(int addr) const {
    // Templated inheritance hides all inherited names - need to access them
    // like this.
    return Storage<T>::data[addr];
  }

  virtual void write(T& newData, int addr) {
    Storage<T>::data[addr] = newData;
  }

/* Constructors and destructors */
  AddressedStorage(int size) : Storage<T>(size) {

  }

  virtual ~AddressedStorage() {

  }

};

#endif /* ADDRESSEDSTORAGE_H_ */
