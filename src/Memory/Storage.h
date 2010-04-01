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

#include <vector>
using std::vector;

template<class T>
class Storage {

//==============================//
// Methods
//==============================//

public:

  // Read a value from the data array.
  virtual T& read() {
    throw "Error: Need to implement read() in this subclass.";
  }

  // Write a value to the data array.
  virtual void write(T& newData) {
    throw "Error: Need to implement write() in this subclass.";
  }

//==============================//
// Constructors and destructors
//==============================//

public:

  Storage(int size) : data(size) {

  }

  virtual ~Storage() {

  }

//==============================//
// Local state
//==============================//

protected:

  vector<T> data;

};

#endif /* STORAGE_H_ */
