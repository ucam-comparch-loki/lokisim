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
#include "../Exceptions/ReadingFromEmptyException.h"
#include "../Exceptions/WritingToFullException.h"
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
      throw ReadingFromEmptyException("buffer");
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
      throw WritingToFullException();
    }
  }

  // Returns the value at the front of the queue, but does not remove it.
  T& peek() {
    if(!isEmpty()) {
      return (this->data[readPos.value()]);
    }
    else {
      throw ReadingFromEmptyException("buffer");
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
  uint16_t remainingSpace() {
    return this->size() - fillCount;
  }

  // Print the contents of the buffer.
  void print() {
    for(uint i=0; i<this->size(); i++) cout << this->data[i] << " ";
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

  Buffer(uint16_t size) :
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
  uint16_t fillCount;

};

#endif /* BUFFER_H_ */
