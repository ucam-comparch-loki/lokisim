/*
 * loki_signal.h
 *
 * A signal which combines the three common pieces of information often
 * communicated between Loki's modules:
 *  1. Data
 *  2. Acknowledgements
 *  3. Validity of the data
 *    - Set to "true" when data is written
 *    - Set to "false" when acknowledgement is received
 *
 * By having all of the information in one place, it is possible to greatly
 * reduce the number of events produced during simulation.
 *
 * In order to make use of the extra information, the loki_signal should be
 * connected to loki_in and loki_out ports. This is not necessary if the extra
 * information is not used, however.
 *
 *  Created on: 15 Dec 2011
 *      Author: db434
 */

#ifndef LOKI_SIGNAL_H_
#define LOKI_SIGNAL_H_

#include "systemc"
#include "loki_signal_ifs.h"

using sc_core::sc_buffer;
using sc_core::sc_event;
using sc_core::sc_object;
using sc_core::sc_signal;

template<class T>
class loki_signal : public sc_buffer<T>, public loki_signal_inout_if<T> {

//============================================================================//
// Methods
//============================================================================//

public:

  // Read the current value in the wire.
  virtual const T& read() const {
    return this->m_cur_val;
  }

  // Write a new value to the wire.
  virtual void write(const T& val) {
    // Copied from sc_buffer (and updated for SystemC 2.3)

#if (SC_VERSION_MAJOR >= 2) || ((SC_VERSION_MAJOR == 2) && (SC_VERSION_MINOR >= 3))
    if ( !sc_signal<T>::policy_type::check_write(this,true) )
      return;
#else
    sc_object* writer = sc_core::sc_get_curr_simcontext()->get_current_writer();
    if (sc_signal<T>::m_writer == 0) {
      sc_signal<T>::m_writer = writer;
    } else if (sc_signal<T>::m_writer != writer) {
      sc_signal_invalid_writer(this, sc_signal<T>::m_writer, writer);
    }
#endif

    this->m_new_val = val;
    this->request_update();
  }

  virtual bool valid() const {
    return validData;
  }

  virtual void ack() {
    // Don't want to acknowledge the same data multiple times.
    assert(validData);

    validData = false;

    // As with data changes, wait a finite amount of time to propagate the ack.
    acknowledgement.notify(sc_core::SC_ZERO_TIME);
  }

  virtual const sc_event& ack_event() const {
    return acknowledgement;
  }

  // The name of this type of component.
  virtual const char* kind() const {
    return "loki_signal";
  }

  virtual const sc_event& value_changed_event() const {
    return sc_buffer<T>::value_changed_event();
  }
  virtual const T& get_data_ref() const {
    return sc_buffer<T>::get_data_ref();
  }
  virtual bool event() const {
    return sc_buffer<T>::event();
  }

protected:

  // Update the value held in this wire (copied from sc_buffer).
  virtual void update() {
    // New for SystemC 2.3
#if (SC_VERSION_MAJOR >= 2) || ((SC_VERSION_MAJOR == 2) && (SC_VERSION_MINOR >= 3))
    sc_signal<T>::policy_type::update();
#endif

    this->m_cur_val = this->m_new_val;
    validData = true;
    if (sc_signal<T>::m_change_event_p)
      sc_signal<T>::m_change_event_p->notify(sc_core::SC_ZERO_TIME);

    // Changed for SystemC 2.3
#if (SC_VERSION_MAJOR >= 2) || ((SC_VERSION_MAJOR == 2) && (SC_VERSION_MINOR >= 3))
    this->m_change_stamp = sc_signal<T>::simcontext()->change_stamp();
#else
    this->m_delta = sc_core::sc_delta_count();
#endif
  }

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  loki_signal() :
      sc_buffer<T>(sc_core::sc_gen_unique_name("loki_signal")),
      acknowledgement(sc_core::sc_gen_unique_name("acknowledgement")) {
    validData = false;
  }

  loki_signal(const sc_core::sc_module_name name) :
      sc_buffer<T>(name),
      acknowledgement(sc_core::sc_gen_unique_name("acknowledgement")) {
    validData = false;
  }

  virtual ~loki_signal() {}

//============================================================================//
// Local state
//============================================================================//

private:

  bool validData;

  // Event which is triggered whenever an acknowledgement is sent.
  sc_event acknowledgement;

};


#endif /* LOKI_SIGNAL_H_ */
