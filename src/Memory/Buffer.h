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

  // Read from the buffer. Returns the oldest value which has not yet been
  // read.
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

  // Write the given data to the buffer.
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

  // Returns the value at the front of the queue, but does not remove it.
  T& peek() {
    if(!isEmpty()) {
      return (Storage<T>::data[readFrom]);
    }
    else {
      std::cerr << "Exception in Buffer.peek()" << std::endl;
      throw std::exception();
    }
  }

  // Removes the element at the front of the queue, without returning it.
  void discardTop() {
    if(!isEmpty()) incrementReadFrom();
  }

  // Returns whether the buffer is empty.
  bool isEmpty() {
    return (fillCount == 0);
  }

  // Returns whether the buffer is full.
  bool isFull() {
    return (fillCount == size);
  }

  // Returns the remaining space in the buffer.
  int remainingSpace() {
    return size-fillCount;
  }

  // Print the contents of the buffer.
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
