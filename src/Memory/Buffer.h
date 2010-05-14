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
#include "../Utility/LoopCounter.h"

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
      int i = readPos.value();
      incrementReadFrom();
      return this->data[i];
    }
    else {
      cerr << "Exception in Buffer.read()" << endl;
      throw std::exception();
    }
  }

  // Write the given data to the buffer.
  virtual void write(const T& newData) {
    if(!isFull()) {
      this->data[writePos.value()] = newData;
      incrementWriteTo();
    }
    else {
      cerr << "Exception in Buffer.write()" << endl;
      throw std::exception();
    }
  }

  // Returns the value at the front of the queue, but does not remove it.
  T& peek() {
    if(!isEmpty()) {
      return (this->data[readPos.value()]);
    }
    else {
      cerr << "Exception in Buffer.peek()" << endl;
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
    return (fillCount == this->size());
  }

  // Returns the remaining space in the buffer.
  int remainingSpace() {
    return this->size() - fillCount;
  }

  // Print the contents of the buffer.
  void print() {
    for(int i=0; i<this->size(); i++) cout << this->data[i] << " ";
    cout << endl;
  }

private:

  void incrementReadFrom() {
    ++readPos;
    fillCount--;
  }

  void incrementWriteTo() {
    ++writePos;
    fillCount++;
  }

//==============================//
// Constructors and destructors
//==============================//

public:

  Buffer(int size) :
      Storage<T>(size),
      readPos(size),
      writePos(size) {
    readPos = writePos = fillCount = 0;
  }

  virtual ~Buffer() {

  }

//==============================//
// Local state
//==============================//

private:

  LoopCounter readPos, writePos;
  int fillCount;

};

#endif /* BUFFER_H_ */
