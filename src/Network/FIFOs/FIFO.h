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

#include "../../LokiComponent.h"
#include "../../Utility/Instrumentation.h"
#include "../../Utility/Instrumentation/FIFO.h"
#include "../../Utility/Assert.h"
#include "../../Utility/LoopCounter.h"

using sc_core::sc_event;

template<class T>
class FIFO: public LokiComponent {

//============================================================================//
// Methods
//============================================================================//

public:

  // Read from the buffer. Returns the oldest value which has not yet been
  // read.
  virtual const T& read() {
    loki_assert(!empty());
    uint i = readPos.value();
    loki_assert(i < size());

    // FIXME: Should we move this to the end?
    incrementReadFrom();
    readEvent_.notify(sc_core::SC_ZERO_TIME);

    if (ENERGY_TRACE) {
      Instrumentation::FIFO::pop(size());
      activeCycle();
    }

    return data[i];
  }

  // Write the given data to the buffer.
  virtual void write(const T& newData) {
    loki_assert(!full());
    loki_assert(writePos.value() < size());
    data[writePos.value()] = newData;
    writeEvent_.notify(sc_core::SC_ZERO_TIME);

    if (ENERGY_TRACE) {
      Instrumentation::FIFO::push(size());
      activeCycle();
    }

    incrementWriteTo();
  }

  // Returns the value at the front of the queue, but does not remove it.
  const T& peek() const {
    loki_assert(!empty());
    loki_assert(readPos.value() < size());
    return data[readPos.value()];
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
    return (fillCount == size());
  }

  // Returns the number of readable items in the buffer.
  uint items() const {
    return fillCount;
  }

  // Returns the maximum number of items this buffer can hold.
  size_t size() const {
    return data.size();
  }

  // Returns the remaining space in the buffer.
  uint remainingSpace() const {
    return size() - fillCount;
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
      int position = (readPos.value() + i) % size();
      cout << data[position] << " ";
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
    if (pos >= size()) {
      if ((uint)(pos + size()) < size())
        pos += size();
      else
        pos -= size();
    }

    loki_assert_with_message(pos < size(), "pos: %d, size: %d", pos, size());
    return data[pos];
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

    if (fillCount >= size())
      fillCount -= size();
  }

  void activeCycle() {
    cycle_count_t currentCycle = Instrumentation::currentCycle();
    if (currentCycle != lastActivity) {
      Instrumentation::FIFO::activeCycle(size());
      lastActivity = currentCycle;
    }
  }

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  FIFO(const sc_module_name& name, size_t size) :
      LokiComponent(name),
      data(size),
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

private:

  vector<T> data;

  LoopCounter readPos, writePos;
  uint16_t fillCount;

  sc_event readEvent_, writeEvent_;

  cycle_count_t lastActivity;

};

#endif /* BUFFERSTORAGE_H_ */
