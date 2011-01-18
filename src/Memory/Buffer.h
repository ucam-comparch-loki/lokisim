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
  virtual const T& read() {
    if(!empty()) {
      int i = readPos.value();
      incrementReadFrom();
      return this->data_[i];
    }
    else {
      throw ReadingFromEmptyException("buffer (" + this->name_ + ")");
    }
  }

  // Write the given data to the buffer.
  virtual void write(const T& newData) {
    if(!full()) {
      this->data_[writePos.value()] = newData;
      incrementWriteTo();
    }
    else {
      cerr << "Exception in Buffer.write()" << endl;
      throw WritingToFullException("buffer (" + this->name_ + ")");
    }
  }

  // Returns the value at the front of the queue, but does not remove it.
  const T& peek() const {
    if(!empty()) {
      return this->data_[readPos.value()];
    }
    else {
      throw ReadingFromEmptyException("buffer (" + this->name_ + ")");
    }
  }

  // Removes the element at the front of the queue, without returning it.
  void discardTop() {
    if(!empty()) incrementReadFrom();
  }

  // Returns whether the buffer is empty.
  bool empty() const {
    return (fillCount == 0);
  }

  // Returns whether the buffer is full.
  bool full() const {
    return (fillCount == this->size());
  }

  // Returns the remaining space in the buffer.
  uint16_t remainingSpace() const {
    return this->size() - fillCount;
  }

  // Print the contents of the buffer.
  void print() const {
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

  Buffer(const uint16_t size, std::string name) :
      Storage<T>(size, name),
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
