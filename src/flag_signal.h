/*
 * flag_signal.h
 *
 * A type of channel (much like sc_signal and sc_buffer) which contains a
 * flag telling whether or not the contained data is new. This is useful
 * in the network when we don't want to keep sending the same value.
 *
 * sc_signal and sc_buffer show that the data is new only for the instant when
 * the data is written. flag_signal retains the new data flag until the data is
 * checked using event().
 *
 *  Created on: 26 Feb 2010
 *      Author: db434
 */

#ifndef FLAG_SIGNAL_H_
#define FLAG_SIGNAL_H_

#include "systemc"

using sc_core::sc_signal;
using sc_core::sc_buffer;
using sc_core::sc_object;

template<class T>
class flag_signal : public sc_buffer<T> {

//==============================//
// Methods
//==============================//

public:

  // Read the current value in the wire.
  virtual const T& read() const {
    return this->m_cur_val;
  }

  // Write a new value to the wire.
  virtual void write(const T& val) {
    // Copied from sc_buffer
    sc_object* writer = sc_core::sc_get_curr_simcontext()->get_current_writer();
    if(sc_signal<T>::m_writer == 0) {
      sc_signal<T>::m_writer = writer;
    } else if(sc_signal<T>::m_writer != writer) {
      sc_signal_invalid_writer(this, sc_signal<T>::m_writer, writer);
    }

    this->m_new_val = val;
    this->request_update();
  }

  // The name of this type of component.
  virtual const char* kind() const {
    return "flag_signal";
  }

  // Determine whether an event has occurred (copied from sc_signal).
  virtual bool event() const {
    bool newData = newDataFlag;

    // Need to update a value from within a const method, so use const_cast.
    const_cast<flag_signal<T>*>(this)->newDataFlag = false;

    return newData || this->simcontext()->event_occurred(this->m_delta);
  }

protected:

  // Update the value held in this wire (copied from sc_buffer).
  virtual void update() {
    this->m_cur_val = this->m_new_val;
    newDataFlag = true;
    if (sc_signal<T>::m_change_event_p)
      sc_signal<T>::m_change_event_p->notify(sc_core::SC_ZERO_TIME);
    this->m_delta = sc_core::sc_delta_count();
  }

//==============================//
// Constructors and destructors
//==============================//

public:

  flag_signal() : sc_buffer<T>(sc_core::sc_gen_unique_name("flag_signal")) {
    newDataFlag = false;
  }

  virtual ~flag_signal() {

  }

//==============================//
// Local state
//==============================//

private:

  // Persistent flag showing whether this signal contains new data.
  bool newDataFlag;

};

#endif /* FLAG_SIGNAL_H_ */
