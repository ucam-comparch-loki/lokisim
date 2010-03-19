/*
 * flag_signal.h
 *
 * A type of channel (much like sc_signal and sc_buffer) which contains a
 * flag telling whether or not the contained data is new. This is useful
 * in the network when we don't want to keep sending the same value.
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

  virtual const T& read() const {
    return this->m_cur_val;
  }

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

  // Note: since read() needs to be const, it is newData() that changes the
  // newDataFlag. In most cases this should be fine, since when using a
  // flag_signal, newData() and read() will be used together, but this may
  // cause some unforeseen oddities.
  bool newData() {
    bool returnVal = *newDataFlag;
    *newDataFlag = false;
    return returnVal;
  }

  virtual const char* kind() const {
    return "flag_signal";
  }

  // Determines whether an event has occurred (copied from sc_signal)
  virtual bool event() const {
    bool newData = *newDataFlag;
    *newDataFlag = false;
    return newData || this->simcontext()->event_occurred(this->m_delta);
  }

protected:

  // Update the value held in this wire (copied from sc_buffer)
  virtual void update() {
    this->m_cur_val = this->m_new_val;
    *newDataFlag = true;
    if (sc_signal<T>::m_change_event_p)
      sc_signal<T>::m_change_event_p->notify(sc_core::SC_ZERO_TIME);
    this->m_delta = sc_core::sc_delta_count();
  }

//==============================//
// Constructors and destructors
//==============================//

public:

  flag_signal() : sc_buffer<T>() {
    newDataFlag = new bool;
    *newDataFlag = false;
  }

  virtual ~flag_signal() {

  }

//==============================//
// Local state
//==============================//

private:

  // Inefficient to have a pointer, but need to modify from within const methods
  // Suggestions of alternative approaches welcome!
  bool* newDataFlag;

};

#endif /* FLAG_SIGNAL_H_ */
