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

#include <iostream>
#include <vector>

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
  virtual T& read() {
    throw "Error: Need to implement read() in this subclass.";
  }

  // Write a value to the data array.
  virtual void write(T& newData) {
    throw "Error: Need to implement write() in this subclass.";
  }

  // Print the contents of this data storage.
  virtual void print(int start=0, int end=size()) const {
    for(int i=start; i<end; i++) cout << i*4 << "\t" << data[i] << endl;
  }

protected:

  // Return the size of this storage component, in [bytes/words].
  int size() const {
    return data.size();
  }

  // Throw an exception if the address is not within the bounds of the array.
  void checkBounds(int addr) const {
    if((addr < 0) || (addr >= size())) {
      cerr << "Error: attempting to access memory address " << addr << endl;
      throw std::exception();
    }
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
