/*
 * Storage.h
 *
 * The base class for all memory-based components. Chip components which have
 * memory should own at least one instance of a Storage.
 *
 * Since this class is templated, all of the implementation has to go in the
 * header file.
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef STORAGE_H_
#define STORAGE_H_

#include <inttypes.h>
#include <iostream>
#include <vector>
#include <assert.h>
#include "../Utility/Parameters.h"
#include "../Exceptions/OutOfBoundsException.h"

using std::vector;
using std::cout;
using std::endl;
using std::cerr;

template<class T>
class Storage {

//==============================//
// Methods
//==============================//

public:

  // Read a value from the data array.
  virtual const T& read() {
    throw "Error: Need to implement read() in this subclass.";
  }

  // Write a value to the data array.
  virtual void write(const T& newData) {
    throw "Error: Need to implement write() in this subclass.";
  }

  // Print the contents of this data storage in the given range.
  virtual void print(const int start=0, const int end=size()) const {
    //for(int i=start; i<end; i++)
    //  cout << i*BYTES_PER_WORD << "\t" << data_[i] << endl;
  }

  // Return the size of this storage component, in [bytes/words].
  uint32_t size() const {
    return data_.size();
  }

  const std::string name() const {
    return name_;
  }

protected:

  // Throw an exception if the address is not within the bounds of the array.
  virtual void checkBounds(const uint32_t addr) const {
//    assert(addr >= 0);
//    assert(addr < size());

    if((addr < 0) || (addr >= size())) {
      throw OutOfBoundsException(addr, size()-1);
    }
  }

//==============================//
// Constructors and destructors
//==============================//

public:

  Storage(const uint32_t size, const std::string& name) : data_(size), name_(name) {
    assert(size > 0);
  }

  virtual ~Storage() {
    // Do nothing
  }

//==============================//
// Local state
//==============================//

protected:

  vector<T>   data_;
  const std::string name_;

};

#endif /* STORAGE_H_ */
