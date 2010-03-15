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

public:
/* Methods */
  virtual T& read() {
    throw "Error: Need to implement read() in this subclass.";
  }

  virtual void write(T& newData) {
    throw "Error: Need to implement write() in this subclass.";
  }

/* Constructors and destructors */
  Storage(int size) : data(size) {

  }

  virtual ~Storage() {

  }

protected:
/* Local state */
  vector<T> data;

};

#endif /* STORAGE_H_ */
