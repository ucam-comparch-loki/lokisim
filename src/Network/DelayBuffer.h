/*
 * DelayBuffer.h
 *
 *  Buffer which does not register as having content until a fixed delay after
 *  the data has been inserted.
 *
 *  This is implemented by adding an extra buffer space for each clock cycle of
 *  delay, and modifying the full/empty signals to check the time at which each
 *  item will reach the underlying buffer.
 *
 *  This models a fully-pipelined system, where it is possible to store one item
 *  in each stage.
 *
 *  Created on: 22 Jun 2015
 *      Author: db434
 */

#ifndef SRC_NETWORK_DELAYBUFFER_H_
#define SRC_NETWORK_DELAYBUFFER_H_

#include "../Component.h"
#include "NetworkBuffer.h"

template<class T>
class DelayBuffer : public Component {

  struct TimedData {
    T       data;       // Data to be written
    double  finishTime; // Time data appears to enter buffer.
  };

public:
  DelayBuffer(const sc_module_name& name, const size_t size, const double delayCycles) :
      Component(name),
      buffer(this->name(), size+int(delayCycles)),
      delay(delayCycles) {

  }
  virtual ~DelayBuffer() {}

  virtual const T& read() {
    const T& data = buffer.read().data;

    // If the head of the queue hasn't yet reached the buffer, trigger an event
    // when it does so.
    if (!trulyEmpty()) {
      if (empty())
        delayedWrite.notify(sc_time(buffer.peek().finishTime - currentTime(), sc_core::SC_NS));
      else
        delayedWrite.notify(sc_core::SC_ZERO_TIME);
    }

    return data;
  }

  virtual const T& peek() {
    return buffer.peek().data;
  }

  void discardTop() {
    buffer.discardTop();
  }

  virtual void write(const T& data) {
    TimedData item;
    item.data = data;
    item.finishTime = currentTime() + delay;

    delayedWrite.notify(sc_time(delay, sc_core::SC_NS));

    buffer.write(item);
  }

  // The buffer is empty if there is no data, or if there is data, but it is
  // not old enough to be read yet.
  bool empty() const {
    return buffer.empty() ||
           buffer.peek().finishTime > currentTime();
  }

  bool full() const {
    return buffer.full();
  }

  // Returns the remaining space in the buffer.
  uint16_t remainingSpace() const {
    return buffer.remainingSpace();
  }

  // Event which is triggered whenever data is read from the buffer.
  const sc_event& readEvent() const {
    return buffer.readEvent();
  }

  // Event which is triggered whenever data is written to the buffer (after
  // the imposed delay).
  const sc_event& writeEvent() const {
    return delayedWrite;
  }

  const sc_event& dataConsumedEvent() const {
    return buffer.dataConsumedEvent();
  }

  // Print the contents of the buffer.
  void print() const {
    buffer.print();
  }

private:

  double currentTime() const {
    return sc_core::sc_time_stamp().to_default_time_units();
  }

  // Returns whether the buffer is empty, regardless of how old the data is.
  bool trulyEmpty() const {
    return buffer.empty();
  }

  NetworkBuffer<TimedData> buffer;

  const double delay;

  // Event which is notified whenever data reaches the modelled buffer.
  sc_event delayedWrite;
};

#endif /* SRC_NETWORK_DELAYBUFFER_H_ */
