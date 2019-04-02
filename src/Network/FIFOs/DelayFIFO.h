/*
 * DelayFIFO.h
 *
 *  FIFO which does not register as having content until a fixed delay after
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

#ifndef SRC_NETWORK_DELAYFIFO_H_
#define SRC_NETWORK_DELAYFIFO_H_

#include "NetworkFIFO.h"
#include <queue>

template<class T>
class DelayFIFO : public NetworkFIFO<T> {

public:
  typedef NetworkFIFO<T> base_class;
  typedef Flit<T> stored_data;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  DelayFIFO(const sc_module_name& name, const size_t size, const double delayCycles) :
      NetworkFIFO<T>(name, size + int(delayCycles)),
      delay(delayCycles) {

  }
  virtual ~DelayFIFO() {}

//============================================================================//
// Methods
//============================================================================//

public:

  virtual const stored_data read() {
    loki_assert(writeTimes.front() <= currentTime());

    const stored_data data = base_class::read();
    writeTimes.pop();

    // If the head of the queue hasn't yet reached the buffer, trigger an event
    // when it does so.
    if (!trulyEmpty()) {
      if (!dataAvailable())
        delayedWrite.notify(sc_time(writeTimes.front() - currentTime(), sc_core::SC_NS));
      else
        delayedWrite.notify(sc_core::SC_ZERO_TIME);
    }

    loki_assert(base_class::items() == writeTimes.size());

    return data;
  }

  virtual void write(const stored_data& data) {
    base_class::write(data);
    writeTimes.push(currentTime() + delay);

    delayedWrite.notify(sc_time(delay, sc_core::SC_NS));

    loki_assert(base_class::items() == writeTimes.size());
  }

  // The buffer is empty if there is no data, or if there is data, but it is
  // not old enough to be read yet.
  virtual bool dataAvailable() const {
    return base_class::dataAvailable() &&
           writeTimes.front() <= currentTime();
  }

  // Event which is triggered whenever data is written to the buffer (after
  // the imposed delay).
  virtual const sc_event& dataAvailableEvent() const {
    return delayedWrite;
  }

private:

  double currentTime() const {
    return sc_core::sc_time_stamp().to_default_time_units();
  }

  // Returns whether the buffer is empty, regardless of how old the data is.
  bool trulyEmpty() const {
    return base_class::fifo.empty();
  }

//============================================================================//
// Local state
//============================================================================//

protected:

  // The times at which each item should become visible in the buffer.
  std::queue<double> writeTimes;

  const double delay;

  // Event which is notified whenever data reaches the modelled buffer.
  sc_event delayedWrite;
};

#endif /* SRC_NETWORK_DELAYFIFO_H_ */
