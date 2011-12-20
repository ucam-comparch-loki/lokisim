/*
 * loki_signal_ifs.h
 *
 * Extensions to sc_signal_ifs which add the extra methods that loki_signals
 * make use of:
 *   * bool valid()
 *   * void ack()
 *   * const sc_event& ack_event()
 *
 *  Created on: 16 Dec 2011
 *      Author: db434
 */

#ifndef LOKI_SIGNAL_IFS_H_
#define LOKI_SIGNAL_IFS_H_


#include "sysc/communication/sc_interface.h"


namespace sc_dt
{
    class sc_logic;
}

class sc_signal_bool_deval;
class sc_signal_logic_deval;

using sc_core::sc_event;
using sc_core::sc_signal_in_if;
using sc_core::sc_signal_inout_if;
using sc_core::sc_signal_write_if;


// ----------------------------------------------------------------------------
//  CLASS : loki_signal_in_if<T>
//
//  The sc_signal<T> input interface class.
// ----------------------------------------------------------------------------

template <class T>
class loki_signal_in_if
: public sc_signal_in_if<T>
{
public:

//    // get the value changed event
//    virtual const sc_event& value_changed_event() const = 0;
//
//
//    // read the current value
//    virtual const T& read() const = 0;
//
//    // get a reference to the current value (for tracing)
//    virtual const T& get_data_ref() const = 0;
//
//
//    // was there a value changed event?
//    virtual bool event() const = 0;

    // Is the data valid?
    virtual bool valid() const = 0;

    // Acknowledge the data.
    virtual void ack() = 0;

protected:

    // constructor

    loki_signal_in_if()
  {}

private:

    // disabled
    loki_signal_in_if( const loki_signal_in_if<T>& );
    loki_signal_in_if<T>& operator = ( const loki_signal_in_if<T>& );
};


// ----------------------------------------------------------------------------
//  CLASS : loki_signal_in_if<bool>
//
//  Specialization of loki_signal_in_if<T> for type bool.
// ----------------------------------------------------------------------------

class sc_reset;

//template <>
//class loki_signal_in_if<bool>
//: public sc_signal_in_if<bool>
//{
//public:
//
//    // get the value changed event
//    virtual const sc_event& value_changed_event() const = 0;
//
//    // get the positive edge event
//    virtual const sc_event& posedge_event() const = 0;
//
//    // get the negative edge event
//    virtual const sc_event& negedge_event() const = 0;
//
//
//    // read the current value
//    virtual const bool& read() const = 0;
//
//    // get a reference to the current value (for tracing)
//    virtual const bool& get_data_ref() const = 0;
//
//
//    // was there a value changed event?
//    virtual bool event() const = 0;
//
//    // was there a positive edge event?
//    virtual bool posedge() const = 0;
//
//    // was there a negative edge event?
//    virtual bool negedge() const = 0;
//
//  // designate this object as a reset signal.
//  virtual sc_reset* is_reset() const = 0;
//
//protected:
//
//    // constructor
//
//    loki_signal_in_if()
//  {}
//
//private:
//
//    // disabled
//    loki_signal_in_if( const loki_signal_in_if<bool>& );
//    loki_signal_in_if<bool>& operator = ( const loki_signal_in_if<bool>& );
//};


// ----------------------------------------------------------------------------
//  CLASS : loki_signal_in_if<sc_dt::sc_logic>
//
//  Specialization of loki_signal_in_if<T> for type sc_dt::sc_logic.
// ----------------------------------------------------------------------------

//template <>
//class loki_signal_in_if<sc_dt::sc_logic>
//: public sc_signal_in_if<sc_dt::sc_logic>
//{
//public:
//
//    // get the value changed event
//    virtual const sc_event& value_changed_event() const = 0;
//
//    // get the positive edge event
//    virtual const sc_event& posedge_event() const = 0;
//
//    // get the negative edge event
//    virtual const sc_event& negedge_event() const = 0;
//
//
//    // read the current value
//    virtual const sc_dt::sc_logic& read() const = 0;
//
//    // get a reference to the current value (for tracing)
//    virtual const sc_dt::sc_logic& get_data_ref() const = 0;
//
//
//    // was there a value changed event?
//    virtual bool event() const = 0;
//
//    // was there a positive edge event?
//    virtual bool posedge() const = 0;
//
//    // was there a negative edge event?
//    virtual bool negedge() const = 0;
//
//
//protected:
//
//    // constructor
//
//    loki_signal_in_if()
//  {}
//
//private:
//
//    // disabled
//    loki_signal_in_if( const loki_signal_in_if<sc_dt::sc_logic>& );
//    loki_signal_in_if<sc_dt::sc_logic>& operator = (
//  const loki_signal_in_if<sc_dt::sc_logic>& );
//};


// ----------------------------------------------------------------------------
//  CLASS : loki_signal_write_if<T>
//
//  The standard output interface class.
// ----------------------------------------------------------------------------
template< typename T >
class loki_signal_write_if : public sc_signal_write_if<T>
{
public:
  loki_signal_write_if() {}
//    // write the new value
//    virtual void write( const T& ) = 0;

    // Is the previously-written data still valid?
//    virtual bool valid() const = 0;

    // Event which is triggered when the destination acknowledges the data.
    virtual const sc_event& ack_event() const = 0;
private:
    // disabled
    loki_signal_write_if( const loki_signal_write_if<T>& );
    loki_signal_write_if<T>& operator = ( const loki_signal_write_if<T>& );
};


// ----------------------------------------------------------------------------
//  CLASS : loki_signal_inout_if<T>
//
//  The sc_signal<T> input/output interface class.
// ----------------------------------------------------------------------------

template <class T>
class loki_signal_inout_if
: public loki_signal_in_if<T>, public loki_signal_write_if<T>
{

protected:

    // constructor

    loki_signal_inout_if()
  {}

private:

    // disabled
    loki_signal_inout_if( const loki_signal_inout_if<T>& );
    loki_signal_inout_if<T>& operator = ( const loki_signal_inout_if<T>& );
};


// ----------------------------------------------------------------------------
//  CLASS : loki_signal_out_if<T>
//
//  The sc_signal<T> output interface class.
// ----------------------------------------------------------------------------

// loki_signal_out_if can also be read from, hence no difference with
// loki_signal_inout_if.

#define loki_signal_out_if loki_signal_inout_if


#endif /* LOKI_SIGNAL_IFS_H_ */
