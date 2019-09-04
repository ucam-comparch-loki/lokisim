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
#include <deque>

using sc_core::sc_time;
using sc_core::SC_NS;

template<class T>
class DelayFIFO : public NetworkFIFO<T> {

public:
  typedef NetworkFIFO<T> base_class;
  typedef Flit<T> stored_data;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(DelayFIFO);

  DelayFIFO(const sc_module_name& name, const fifo_parameters_t& params,
    const double delayCycles) :
      NetworkFIFO<T>(name, params.size + int(delayCycles), params.bandwidth),
      delay(delayCycles) {

    SC_METHOD(delayedWriteEvent);

  }

  DelayFIFO(const sc_module_name& name, size_t size, bandwidth_t bandwidth,
    const double delayCycles) :
      NetworkFIFO<T>(name, size + int(delayCycles), bandwidth),
      delay(delayCycles) {

    SC_METHOD(delayedWriteEvent);

  }

  virtual ~DelayFIFO() {}

//============================================================================//
// Methods
//============================================================================//

public:

  virtual const stored_data read() {
    loki_assert(writeTimes.front().time <= currentTime());

    const stored_data data = base_class::read();
    writeTimes.pop_front();

    loki_assert(base_class::items() == writeTimes.size());

    return data;
  }

  virtual void write(const stored_data& data) {
    base_class::write(data);
    
    // Record when this write should become visible to others.
    EventStatus status;
    status.time = currentTime() + delay;
    status.triggered = false;
    writeTimes.push_back(status);

    loki_assert(base_class::items() == writeTimes.size());
  }

  // Event which is triggered whenever data is written to the buffer (after
  // the imposed delay).
  virtual const sc_event& writeEvent() const {
    return delayedWrite;
  }

protected:

  // The buffer is empty if there is no data, or if there is data, but it is
  // not old enough to be read yet.
  virtual bool empty() const {
    return base_class::empty() ||
           writeTimes.front().time > currentTime();
  }

private:

  double currentTime() const {
    return sc_core::sc_time_stamp().to_default_time_units();
  }

  // Ensure delayedWrite is triggered at the correct times, and the correct
  // number of times. SystemC only allows one pending notification on each
  // event, so I need to track this manually to handle bursts of writes.
  void delayedWriteEvent() {
    // Find the first untriggered event, and trigger it. Add a delta cycle if
    // necessary to avoid triggering multiple times simultaneously.
    for (auto it=writeTimes.begin(); it != writeTimes.end(); ++it) {
      if (!it->triggered) {
        if (it->time == currentTime())
          delayedWrite.notify(sc_core::SC_ZERO_TIME);
        else
          delayedWrite.notify(it->time - currentTime(), SC_NS);

        it->triggered = true;
        next_trigger(delayedWrite);
        return;
      }
    }

    // Default trigger: new data written to FIFO.
    next_trigger(base_class::writeEvent());
  }

  // Returns whether the buffer is empty, regardless of how old the data is.
  bool trulyEmpty() const {
    return base_class::empty();
  }

//============================================================================//
// Local state
//============================================================================//

protected:

  typedef struct {
    double time;      // The time when an event should be triggered.
    bool   triggered; // Whether this event has been triggered (can't trigger
                      // multiple simultaneously).
  } EventStatus;

  // The times at which each item should become visible in the buffer.
  std::deque<EventStatus> writeTimes;

  const double delay;

  // Event which is notified whenever data reaches the modelled buffer.
  sc_event delayedWrite;
};

#endif /* SRC_NETWORK_DELAYFIFO_H_ */
