/*
 * FIFO.h
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

#include "../../Memory/Storage.h"
#include "systemc.h"
#include "../../Utility/Instrumentation.h"
#include "../../Utility/Instrumentation/FIFO.h"
#include "../../Utility/LoopCounter.h"

using sc_core::sc_event;

template<class T>
class FIFO: public Storage<T> {

//============================================================================//
// Methods
//============================================================================//

public:

  // Read from the buffer. Returns the oldest value which has not yet been
  // read.
  virtual const T& read() {
    assert(!empty());
    int i = readPos.value();
    // FIXME: Should we move this to the end?
    incrementReadFrom();
    this->readEvent_.notify(sc_core::SC_ZERO_TIME);

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
    this->writeEvent_.notify(sc_core::SC_ZERO_TIME);

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

  // Returns the number of readable items in the buffer.
  uint items() const {
    return fillCount;
  }

  // Returns the remaining space in the buffer.
  uint remainingSpace() const {
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
  uint getReadPointer() const     {return readPos.value();}
  uint getWritePointer() const    {return writePos.value();}
  void setReadPointer(uint pos)   {readPos = pos;  updateFillCount();}
  void setWritePointer(uint pos)  {writePos = pos; updateFillCount();}

  const T& debugRead(uint pos) const {
    // Dealing with possible underflow.
    if (pos >= this->size()) {
      if (pos + this->size() < this->size())
        pos += this->size();
      else
        pos -= this->size();
    }

    return this->data_[pos];
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

  void updateFillCount() {
    fillCount = writePos - readPos;

    if (fillCount >= this->size())
      fillCount -= this->size();
  }

  void activeCycle() {
    cycle_count_t currentCycle = Instrumentation::currentCycle();
    if (currentCycle != lastActivity) {
      Instrumentation::FIFO::activeCycle(this->size());
      lastActivity = currentCycle;
    }
  }

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  FIFO(const std::string& name, const size_t size) :
      Storage<T>(name, size),
      readPos(size),
      writePos(size) {
    readPos = writePos = fillCount = 0;
    lastActivity = -1;

    Instrumentation::FIFO::createdFIFO(size);
  }

  virtual ~FIFO() {}

//============================================================================//
// Local state
//============================================================================//

protected:

  LoopCounter readPos, writePos;
  uint16_t fillCount;

  sc_event readEvent_, writeEvent_;

  cycle_count_t lastActivity;

};

#endif /* BUFFERSTORAGE_H_ */
