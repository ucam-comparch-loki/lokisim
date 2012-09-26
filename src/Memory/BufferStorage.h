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

#ifndef BUFFERSTORAGE_H_
#define BUFFERSTORAGE_H_

#include "Storage.h"
#include "systemc.h"
#include "../Utility/Instrumentation.h"
#include "../Utility/Instrumentation/FIFO.h"
#include "../Utility/LoopCounter.h"

using sc_core::sc_event;

template<class T>
class BufferStorage: public Storage<T> {

//==============================//
// Methods
//==============================//

public:

  // Read from the buffer. Returns the oldest value which has not yet been
  // read.
  virtual const T& read() {
    assert(!empty());
    int i = readPos.value();
    incrementReadFrom();
    this->readEvent_.notify();

    if (ENERGY_TRACE) {
      Instrumentation::FIFO::pop(this->size());
      activeCycle();
    }

    return this->data_[i];
  }

  // Write the given data to the buffer.
  virtual void write(const T& newData) {
    assert(!full());
    this->data_[writePos.value()] = newData;
    this->writeEvent_.notify();

    if (ENERGY_TRACE) {
      Instrumentation::FIFO::push(this->size());
      activeCycle();
    }

    incrementWriteTo();
  }

  // Returns the value at the front of the queue, but does not remove it.
  const T& peek() const {
    assert(!empty());
    return this->data_[readPos.value()];
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

  // Event which is triggered whenever data is read from the buffer.
  const sc_event& readEvent() const {
    return readEvent_;
  }

  // Event which is triggered whenever data is written to the buffer.
  const sc_event& writeEvent() const {
    return writeEvent_;
  }

  // Print the contents of the buffer.
  void print() const {
    for(uint i=0; i<fillCount; i++) {
      int position = (readPos.value() + i) % this->size();
      cout << this->data_[position] << " ";
    }
    cout << endl;
  }

  // Low-level access methods - use with caution!
  uint getReadPointer()           {return readPos.value();}
  uint getWritePointer()          {return writePos.value();}
  void setReadPointer(uint pos)   {readPos = pos;}
  void setWritePointer(uint pos)  {writePos = pos;}

private:

  void incrementReadFrom() {
    ++readPos;
    fillCount--;
  }

  void incrementWriteTo() {
    ++writePos;
    fillCount++;
  }

  void activeCycle() {
    cycle_count_t currentCycle = Instrumentation::currentCycle();
    if (currentCycle != lastActivity) {
      Instrumentation::FIFO::activeCycle(this->size());
      lastActivity = currentCycle;
    }
  }

//==============================//
// Constructors and destructors
//==============================//

public:

  BufferStorage(const uint16_t size, const std::string& name) :
      Storage<T>(size, name),
      readPos(size),
      writePos(size) {
    readPos = writePos = fillCount = 0;
    lastActivity = -1;

    Instrumentation::FIFO::createdFIFO(size);
  }

//==============================//
// Local state
//==============================//

private:

  LoopCounter readPos, writePos;
  uint16_t fillCount;

  sc_event readEvent_, writeEvent_;

  cycle_count_t lastActivity;

};

#endif /* BUFFERSTORAGE_H_ */
