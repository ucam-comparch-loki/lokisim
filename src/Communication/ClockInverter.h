/*
 * ClockInverter.h
 *
 * Invert a clock signal.
 *
 * To use, replace `clock_port(clock)`
 * with `clock_inverter(clock)` and `clock_port(clock_inverter)`.
 *
 * The implementation is currently imperfect and event finders (`pos()` and
 * `neg()`) do not work as expected. Any component which uses the output of
 * an inverted clock should only declare sensitivity in an
 * `end_of_elaboration()` method.
 *
 *  Created on: Sep 10, 2019
 *      Author: db434
 */

#ifndef SRC_COMMUNICATION_CLOCKINVERTER_H_
#define SRC_COMMUNICATION_CLOCKINVERTER_H_

#include "../LokiComponent.h"
#include "../Types.h"

using sc_core::sc_event;

class ClockInverter : public LokiComponent, public sc_signal_in_if<bool> {

public:
  ClockInverter(sc_module_name name) :
      LokiComponent(name),
      sc_signal_in_if<bool>(),
      original("clock") {
    // Nothing
  }

  // Bind to a clock signal.
  void operator () (ClockInput& clock) {
    original.bind(clock);
  }

  // Just switch all of the edge-specific functions around.

  // get the default event
  virtual const sc_event& default_event() const {
    return original.default_event();
  }

  // get the value changed event
  virtual const sc_event& value_changed_event() const {
    return original.value_changed_event();
  }

  // get the positive edge event
  virtual const sc_event& posedge_event() const {
    return original.negedge_event();
  }

  // get the negative edge event
  virtual const sc_event& negedge_event() const {
    return original.posedge_event();
  }


  // read the current value
  virtual const bool& read() const {
    // This is not straight-forward due to the combination of:
    //  1. Needing to modify (invert) the return value.
    //  2. Needing to return a reference (so need a class member).
    //  3. This being a const method (so can't modify that class member).
    // Could have a separate thread which watches the original value and updates
    // a class member when it changes, but that seems like overkill.
    assert(false);
    return original.read();
  }

  // get a reference to the current value (for tracing)
  virtual const bool& get_data_ref() const {
    return read();
  }


  // was there a value changed event?
  virtual bool event() const {
    return original.event();
  }

  // was there a positive edge event?
  virtual bool posedge() const {
    return original.negedge();
  }

  // was there a negative edge event?
  virtual bool negedge() const {
    return original.posedge();
  }

private:

  ClockInput original;

};



#endif /* SRC_COMMUNICATION_CLOCKINVERTER_H_ */
