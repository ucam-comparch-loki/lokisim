/*
 * Buffer.h
 *
 * A simple circular buffer with FIFO semantics.
 *
 * Since this class is templated, all of the implementation must go in the
 * header file.
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#include "Storage.h"

template<class T>
class Buffer: public Storage<T> {

//==============================//
// Methods
//==============================//

public:

  virtual T& read() {
    if(!isEmpty()) {
      int i = readFrom;
      incrementReadFrom();
      return Storage<T>::data[i];
    }
    else {
      std::cerr << "Exception in Buffer.read()" << std::endl;
      throw std::exception();
    }
  }

  virtual void write(const T& newData) {
    if(!isFull()) {
      Storage<T>::data[writeTo] = newData;
      incrementWriteTo();
    }
    else {
      std::cerr << "Exception in Buffer.write()" << std::endl;
      throw std::exception();
    }
  }

  T& peek() {
    if(!isEmpty()) {
      return (Storage<T>::data[readFrom]);
    }
    else {
      std::cerr << "Exception in Buffer.peek()" << std::endl;
      throw std::exception();
    }
  }

  void discardTop() {
    if(!isEmpty()) incrementReadFrom();
  }

  bool isEmpty() {
    return (fillCount == 0);
  }

  bool isFull() {
    return (fillCount == size);
  }

  int remainingSpace() {
    return size-fillCount;
  }

  void print() {
    for(int i=0; i<this->size; i++) std::cout << Storage<T>::data[i] << " ";
    std::cout << std::endl;
  }

private:

  void incrementReadFrom() {
    readFrom = (readFrom >= size-1) ? 0 : readFrom+1;
    fillCount--;
  }

  void incrementWriteTo() {
    writeTo = (writeTo >= size-1) ? 0 : writeTo+1;
    fillCount++;
  }

//==============================//
// Constructors and destructors
//==============================//

public:

  Buffer(int size) : Storage<T>(size) {
    readFrom = writeTo = fillCount = 0;
    this->size = size;
  }

  virtual ~Buffer() {

  }

//==============================//
// Local state
//==============================//

private:

  int readFrom, writeTo, fillCount, size;

};

#endif /* BUFFER_H_ */
