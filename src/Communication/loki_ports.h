/*
 * loki_ports.h
 *
 * Ports which connect to loki_signals, and are therefore capable of dealing
 * with "valid" and "acknowledge" signals at the same time as the data.
 *
 * Implementation is copied from sc_signal_ports.h, where sc_in and sc_out are
 * defined.
 *
 *  Created on: 15 Dec 2011
 *      Author: db434
 */

#ifndef LOKI_PORTS_H_
#define LOKI_PORTS_H_

#include "systemc"
#include "loki_signal_ifs.h"

using sc_core::sc_event_finder;
using sc_core::sc_event_finder_t;
using sc_core::sc_in;
using sc_core::sc_interface;
using sc_core::sc_out;
using sc_core::sc_port;
using sc_core::sc_port_b;
using sc_core::sc_port_base;
using sc_core::sc_trace;
using sc_core::sc_trace_file;
using sc_core::sc_trace_params;
using sc_core::sc_trace_params_vec;
using sc_core::SC_ONE_OR_MORE_BOUND;


// ----------------------------------------------------------------------------
//  CLASS : loki_in<T>
//
//  The loki_signal<T> input port class.
// ----------------------------------------------------------------------------

template <class T>
class loki_in
: public sc_port<loki_signal_in_if<T>,1,SC_ONE_OR_MORE_BOUND>
{
public:

    // typedefs

    typedef T                                             data_type;

    typedef loki_signal_in_if<data_type>                    if_type;
    typedef sc_port<if_type,1,SC_ONE_OR_MORE_BOUND>       base_type;
    typedef loki_in<data_type>                              this_type;

    typedef if_type                                       in_if_type;
    typedef base_type                                     in_port_type;
    typedef loki_signal_inout_if<data_type>                 inout_if_type;
    typedef sc_port<inout_if_type,1,SC_ONE_OR_MORE_BOUND> inout_port_type;

public:

    // constructors

    loki_in()
  : base_type(), m_traces( 0 ),
    m_change_finder_p(0)
  {}

    explicit loki_in( const char* name_ )
  : base_type( name_ ), m_traces( 0 ),
    m_change_finder_p(0)
  {}

    explicit loki_in( const in_if_type& interface_ )
        : base_type( const_cast<in_if_type&>( interface_ ) ), m_traces( 0 ),
    m_change_finder_p(0)
        {}

    loki_in( const char* name_, const in_if_type& interface_ )
  : base_type( name_, const_cast<in_if_type&>( interface_ ) ), m_traces( 0 ),
    m_change_finder_p(0)
  {}

    explicit loki_in( in_port_type& parent_ )
  : base_type( parent_ ), m_traces( 0 ),
    m_change_finder_p(0)
  {}

    loki_in( const char* name_, in_port_type& parent_ )
  : base_type( name_, parent_ ), m_traces( 0 ),
    m_change_finder_p(0)
  {}

    explicit loki_in( inout_port_type& parent_ )
  : base_type(), m_traces( 0 ),
    m_change_finder_p(0)
  { sc_port_base::bind( parent_ ); }

    loki_in( const char* name_, inout_port_type& parent_ )
  : base_type( name_ ), m_traces( 0 ),
    m_change_finder_p(0)
  { sc_port_base::bind( parent_ ); }

    loki_in( this_type& parent_ )
  : base_type( parent_ ), m_traces( 0 ),
    m_change_finder_p(0)
  {}

    loki_in( const char* name_, this_type& parent_ )
  : base_type( name_, parent_ ), m_traces( 0 ),
    m_change_finder_p(0)
  {}


    // destructor

    virtual ~loki_in()
  {
      remove_traces();
      if ( m_change_finder_p ) delete m_change_finder_p;
  }


    // bind to in interface

    void bind( const in_if_type& interface_ )
  { sc_port_base::bind( const_cast<in_if_type&>( interface_ ) ); }

    void operator () ( const in_if_type& interface_ )
  { sc_port_base::bind( const_cast<in_if_type&>( interface_ ) ); }


    // bind to parent in port

    void bind( in_port_type& parent_ )
        { sc_port_base::bind( parent_ ); }

    void operator () ( in_port_type& parent_ )
        { sc_port_base::bind( parent_ ); }


    // bind to parent inout port

    void bind( inout_port_type& parent_ )
  { sc_port_base::bind( parent_ ); }

    void operator () ( inout_port_type& parent_ )
  { sc_port_base::bind( parent_ ); }


    // interface access shortcut methods

    // get the default event

    const sc_event& default_event() const
  { return (*this)->default_event(); }


    // get the value changed event

    const sc_event& value_changed_event() const
  { return (*this)->value_changed_event(); }


    // read the current value

    const data_type& read() const
  { return (*this)->read(); }

    operator const data_type& () const
  { return (*this)->read(); }


    // was there a value changed event?

    bool event() const
  { return (*this)->event(); }


    // Is the data at the port currently valid?
    bool valid() const {
      return (*this)->valid();
    }

    // Acknowledge the data.
    void ack() {
      (*this)->ack();
    }


    // (other) event finder method(s)

    sc_event_finder& value_changed() const
    {
        if ( !m_change_finder_p )
  {
      m_change_finder_p = new sc_event_finder_t<in_if_type>(
          *this, &in_if_type::value_changed_event );
  }
  return *m_change_finder_p;
    }


    // called when elaboration is done
    /*  WHEN DEFINING THIS METHOD IN A DERIVED CLASS, */
    /*  MAKE SURE THAT THIS METHOD IS CALLED AS WELL. */

    virtual void end_of_elaboration();

    virtual const char* kind() const
        { return "loki_in"; }


    void add_trace( sc_trace_file*, const std::string& ) const;

    // called by sc_trace
    void add_trace_internal( sc_trace_file*, const std::string& ) const;

protected:

    void remove_traces() const;

    mutable sc_trace_params_vec* m_traces;

protected:

    // called by pbind (for internal use only)
    virtual int vbind( sc_interface& );
    virtual int vbind( sc_port_base& );

private:
  mutable sc_event_finder* m_change_finder_p;

private:

    // disabled
    loki_in( const this_type& );
    this_type& operator = ( const this_type& );

#ifdef __GNUC__
    // Needed to circumvent a problem in the g++-2.95.2 compiler:
    // This unused variable forces the compiler to instantiate
    // an object of T template so an implicit conversion from
    // read() to a C++ intrinsic data type will work.
    static data_type dummy;
#endif
};

template<typename T>
::std::ostream& operator << ( ::std::ostream& os, const loki_in<T>& a )
{
    return os << a->read();
}

// IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII


// called when elaboration is done

template <class T>
inline
void
loki_in<T>::end_of_elaboration()
{
    if( m_traces != 0 ) {
  for( int i = 0; i < (int)m_traces->size(); ++ i ) {
      sc_trace_params* p = (*m_traces)[i];
      in_if_type* iface = dynamic_cast<in_if_type*>( this->get_interface() );
      sc_trace( p->tf, iface->read(), p->name );
  }
  remove_traces();
    }
}


// called by sc_trace

template <class T>
inline
void
loki_in<T>::add_trace_internal( sc_trace_file* tf_, const std::string& name_ )
const
{
    if( tf_ != 0 ) {
  if( m_traces == 0 ) {
      m_traces = new sc_trace_params_vec;
  }
  m_traces->push_back( new sc_trace_params( tf_, name_ ) );
    }
}

template <class T>
inline
void
loki_in<T>::add_trace( sc_trace_file* tf_, const std::string& name_ )
const
{
    sc_core::sc_deprecated_add_trace();
    add_trace_internal(tf_, name_);
}

template <class T>
inline
void
loki_in<T>::remove_traces() const
{
    if( m_traces != 0 ) {
  for( int i = (int)m_traces->size() - 1; i >= 0; -- i ) {
      delete (*m_traces)[i];
  }
  delete m_traces;
  m_traces = 0;
    }
}


// called by pbind (for internal use only)

template <class T>
inline
int
loki_in<T>::vbind( sc_interface& interface_ )
{
    return sc_port_b<if_type>::vbind( interface_ );
}

template <class T>
inline
int
loki_in<T>::vbind( sc_port_base& parent_ )
{
    in_port_type* in_parent = dynamic_cast<in_port_type*>( &parent_ );
    if( in_parent != 0 ) {
  sc_port_base::bind( *in_parent );
  return 0;
    }
    inout_port_type* inout_parent = dynamic_cast<inout_port_type*>( &parent_ );
    if( inout_parent != 0 ) {
  sc_port_base::bind( *inout_parent );
  return 0;
    }
    // type mismatch
    return 2;
}


// ----------------------------------------------------------------------------
//  CLASS : loki_in<bool>
//
//  Specialization of loki_in<T> for type bool.
// ----------------------------------------------------------------------------

template <>
class loki_in<bool> :
    public sc_port<loki_signal_in_if<bool>,1,SC_ONE_OR_MORE_BOUND>
{
public:

    // typedefs

    typedef bool                                           data_type;

    typedef loki_signal_in_if<data_type>                     if_type;
    typedef sc_port<if_type,1,SC_ONE_OR_MORE_BOUND>        base_type;
    typedef loki_in<data_type>                               this_type;

    typedef if_type                                        in_if_type;
    typedef base_type                                      in_port_type;
    typedef loki_signal_inout_if<data_type>                  inout_if_type;
    typedef sc_port<inout_if_type,1,SC_ONE_OR_MORE_BOUND>  inout_port_type;

public:

    // constructors

    loki_in()
  : base_type(), m_traces( 0 ), m_change_finder_p(0),
    m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    explicit loki_in( const char* name_ )
  : base_type( name_ ), m_traces( 0 ), m_change_finder_p(0),
    m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    explicit loki_in( const in_if_type& interface_ )
  : base_type( const_cast<in_if_type&>( interface_ ) ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    loki_in( const char* name_, const in_if_type& interface_ )
  : base_type( name_, const_cast<in_if_type&>( interface_ ) ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    explicit loki_in( in_port_type& parent_ )
  : base_type( parent_ ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    loki_in( const char* name_, in_port_type& parent_ )
  : base_type( name_, parent_ ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    explicit loki_in( inout_port_type& parent_ )
  : base_type(), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  { sc_port_base::bind( parent_ ); }

    loki_in( const char* name_, inout_port_type& parent_ )
  : base_type( name_ ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  { sc_port_base::bind( parent_ ); }

    loki_in( this_type& parent_ )
  : base_type( parent_ ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

#if defined(TESTING)
    loki_in( const this_type& parent_ )
  : base_type( *(in_if_type*)parent_.get_interface() ) , m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}
#endif

    loki_in( const char* name_, this_type& parent_ )
  : base_type( name_, parent_ ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}


    // destructor

    virtual ~loki_in()
  {
      remove_traces();
      if ( m_change_finder_p ) delete m_change_finder_p;
      if ( m_neg_finder_p ) delete m_neg_finder_p;
      if ( m_pos_finder_p ) delete m_pos_finder_p;
  }


    // bind to in interface

    void bind( const in_if_type& interface_ )
  { sc_port_base::bind( const_cast<in_if_type&>( interface_ ) ); }

    void operator () ( const in_if_type& interface_ )
  { sc_port_base::bind( const_cast<in_if_type&>( interface_ ) ); }


    // bind to parent in port

    void bind( in_port_type& parent_ )
        { sc_port_base::bind( parent_ ); }

    void operator () ( in_port_type& parent_ )
        { sc_port_base::bind( parent_ ); }


    // bind to parent inout port

    void bind( inout_port_type& parent_ )
  { sc_port_base::bind( parent_ ); }

    void operator () ( inout_port_type& parent_ )
  { sc_port_base::bind( parent_ ); }


    // interface access shortcut methods

    // get the default event

    const sc_event& default_event() const
  { return (*this)->default_event(); }


    // get the value changed event

    const sc_event& value_changed_event() const
  { return (*this)->value_changed_event(); }

    // get the positive edge event

    const sc_event& posedge_event() const
  { return (*this)->posedge_event(); }

    // get the negative edge event

    const sc_event& negedge_event() const
  { return (*this)->negedge_event(); }


    // read the current value

    const data_type& read() const
  { return (*this)->read(); }

    operator const data_type& () const
  { return (*this)->read(); }


    // use for positive edge sensitivity

    sc_event_finder& pos() const
    {
        if ( !m_pos_finder_p )
  {
      m_pos_finder_p = new sc_event_finder_t<in_if_type>(
          *this, &in_if_type::posedge_event );
  }
  return *m_pos_finder_p;
    }

    // use for negative edge sensitivity

    sc_event_finder& neg() const
    {
        if ( !m_neg_finder_p )
  {
      m_neg_finder_p = new sc_event_finder_t<in_if_type>(
          *this, &in_if_type::negedge_event );
  }
  return *m_neg_finder_p;
    }


    // was there a value changed event?

    bool event() const
  { return (*this)->event(); }

    // was there a positive edge event?

    bool posedge() const
        { return (*this)->posedge(); }

    // was there a negative edge event?

    bool negedge() const
        { return (*this)->negedge(); }

    // (other) event finder method(s)

    sc_event_finder& value_changed() const
    {
        if ( !m_change_finder_p )
  {
      m_change_finder_p = new sc_event_finder_t<in_if_type>(
          *this, &in_if_type::value_changed_event );
  }
  return *m_change_finder_p;
    }


    // called when elaboration is done
    /*  WHEN DEFINING THIS METHOD IN A DERIVED CLASS, */
    /*  MAKE SURE THAT THIS METHOD IS CALLED AS WELL. */

    virtual void end_of_elaboration();

    virtual const char* kind() const
        { return "loki_in"; }


    void add_trace( sc_trace_file*, const std::string& ) const;

    // called by sc_trace
    void add_trace_internal( sc_trace_file*, const std::string& ) const;

protected:

    void remove_traces() const;

    mutable sc_trace_params_vec* m_traces;

protected:

    // called by pbind (for internal use only)
    virtual int vbind( sc_interface& );
    virtual int vbind( sc_port_base& );

private:
  mutable sc_event_finder* m_change_finder_p;
  mutable sc_event_finder* m_neg_finder_p;
  mutable sc_event_finder* m_pos_finder_p;

private:

    // disabled
#if defined(TESTING)
#else
    loki_in( const this_type& );
#endif
    this_type& operator = ( const this_type& );

#ifdef __GNUC__
    // Needed to circumvent a problem in the g++-2.95.2 compiler:
    // This unused variable forces the compiler to instantiate
    // an object of T template so an implicit conversion from
    // read() to a C++ intrinsic data type will work.
    static data_type dummy;
#endif
};


// ----------------------------------------------------------------------------
//  CLASS : loki_in<sc_dt::sc_logic>
//
//  Specialization of loki_in<T> for type sc_dt::sc_logic.
// ----------------------------------------------------------------------------

template <>
class loki_in<sc_dt::sc_logic>
: public sc_port<loki_signal_in_if<sc_dt::sc_logic>,1,SC_ONE_OR_MORE_BOUND>
{
public:

    // typedefs

    typedef sc_dt::sc_logic                               data_type;

    typedef loki_signal_in_if<data_type>                    if_type;
    typedef sc_port<if_type,1,SC_ONE_OR_MORE_BOUND>       base_type;
    typedef loki_in<data_type>                              this_type;

    typedef if_type                                       in_if_type;
    typedef base_type                                     in_port_type;
    typedef loki_signal_inout_if<data_type>                 inout_if_type;
    typedef sc_port<inout_if_type,1,SC_ONE_OR_MORE_BOUND> inout_port_type;

public:

    // constructors

    loki_in()
  : base_type(), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    explicit loki_in( const char* name_ )
  : base_type( name_ ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    explicit loki_in( const in_if_type& interface_ )
  : base_type( const_cast<in_if_type&>( interface_ ) ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    loki_in( const char* name_, const in_if_type& interface_ )
  : base_type( name_, const_cast<in_if_type&>( interface_ ) ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    explicit loki_in( in_port_type& parent_ )
  : base_type( parent_ ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    loki_in( const char* name_, in_port_type& parent_ )
  : base_type( name_, parent_ ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    explicit loki_in( inout_port_type& parent_ )
  : base_type(), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  { sc_port_base::bind( parent_ ); }

    loki_in( const char* name_, inout_port_type& parent_ )
  : base_type( name_ ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  { sc_port_base::bind( parent_ ); }

    loki_in( this_type& parent_ )
  : base_type( parent_ ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    loki_in( const char* name_, this_type& parent_ )
  : base_type( name_, parent_ ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}


    // destructor

    virtual ~loki_in()
  {
      remove_traces();
      if ( m_change_finder_p ) delete m_change_finder_p;
      if ( m_neg_finder_p ) delete m_neg_finder_p;
      if ( m_pos_finder_p ) delete m_pos_finder_p;
  }


    // bind to in interface

    void bind( const in_if_type& interface_ )
  { sc_port_base::bind( const_cast<in_if_type&>( interface_ ) ); }

    void operator () ( const in_if_type& interface_ )
  { sc_port_base::bind( const_cast<in_if_type&>( interface_ ) ); }


    // bind to parent in port

    void bind( in_port_type& parent_ )
        { sc_port_base::bind( parent_ ); }

    void operator () ( in_port_type& parent_ )
        { sc_port_base::bind( parent_ ); }


    // bind to parent inout port

    void bind( inout_port_type& parent_ )
  { sc_port_base::bind( parent_ ); }

    void operator () ( inout_port_type& parent_ )
  { sc_port_base::bind( parent_ ); }


    // interface access shortcut methods

    // get the default event

    const sc_event& default_event() const
  { return (*this)->default_event(); }


    // get the value changed event

    const sc_event& value_changed_event() const
  { return (*this)->value_changed_event(); }

    // get the positive edge event

    const sc_event& posedge_event() const
  { return (*this)->posedge_event(); }

    // get the negative edge event

    const sc_event& negedge_event() const
  { return (*this)->negedge_event(); }


    // read the current value

    const data_type& read() const
  { return (*this)->read(); }

    operator const data_type& () const
  { return (*this)->read(); }


    // use for positive edge sensitivity

    sc_event_finder& pos() const
    {
        if ( !m_pos_finder_p )
  {
      m_pos_finder_p = new sc_event_finder_t<in_if_type>(
          *this, &in_if_type::posedge_event );
  }
  return *m_pos_finder_p;
    }

    // use for negative edge sensitivity

    sc_event_finder& neg() const
    {
        if ( !m_neg_finder_p )
  {
      m_neg_finder_p = new sc_event_finder_t<in_if_type>(
          *this, &in_if_type::negedge_event );
  }
  return *m_neg_finder_p;
    }


    // was there a value changed event?

    bool event() const
  { return (*this)->event(); }

    // was there a positive edge event?

    bool posedge() const
        { return (*this)->posedge(); }

    // was there a negative edge event?

    bool negedge() const
        { return (*this)->negedge(); }

    // (other) event finder method(s)

    sc_event_finder& value_changed() const
    {
        if ( !m_change_finder_p )
  {
      m_change_finder_p = new sc_event_finder_t<in_if_type>(
          *this, &in_if_type::value_changed_event );
  }
  return *m_change_finder_p;
    }


    // called when elaboration is done
    /*  WHEN DEFINING THIS METHOD IN A DERIVED CLASS, */
    /*  MAKE SURE THAT THIS METHOD IS CALLED AS WELL. */

    virtual void end_of_elaboration();

    virtual const char* kind() const
        { return "loki_in"; }


    void add_trace( sc_trace_file*, const std::string& ) const;

    // called by sc_trace
    void add_trace_internal( sc_trace_file*, const std::string& ) const;

protected:

    void remove_traces() const;

    mutable sc_trace_params_vec* m_traces;

protected:

    // called by pbind (for internal use only)
    virtual int vbind( sc_interface& );
    virtual int vbind( sc_port_base& );

private:
  mutable sc_event_finder* m_change_finder_p;
  mutable sc_event_finder* m_neg_finder_p;
  mutable sc_event_finder* m_pos_finder_p;

private:

    // disabled
    loki_in( const this_type& );
    this_type& operator = ( const this_type& );

#ifdef __GNUC__
    // Needed to circumvent a problem in the g++-2.95.2 compiler:
    // This unused variable forces the compiler to instantiate
    // an object of T template so an implicit conversion from
    // read() to a C++ intrinsic data type will work.
    static data_type dummy;
#endif
};


// ----------------------------------------------------------------------------
//  CLASS : loki_inout<T>
//
//  The loki_signal<T> input/output port class.
// ----------------------------------------------------------------------------

template <class T>
class loki_inout
: public sc_port<loki_signal_inout_if<T>,1,SC_ONE_OR_MORE_BOUND>
{
public:

    // typedefs

    typedef T                                          data_type;

    typedef loki_signal_inout_if<data_type>              if_type;
    typedef sc_port<if_type,1,SC_ONE_OR_MORE_BOUND>    base_type;
    typedef loki_inout<data_type>                        this_type;

    typedef loki_signal_in_if<data_type>                 in_if_type;
    typedef sc_port<in_if_type,1,SC_ONE_OR_MORE_BOUND> in_port_type;
    typedef if_type                                    inout_if_type;
    typedef base_type                                  inout_port_type;

public:

    // constructors

    loki_inout()
  : base_type(), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_ack_finder_p(0)
  {}

    explicit loki_inout( const char* name_ )
  : base_type( name_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_ack_finder_p(0)
  {}

    explicit loki_inout( inout_if_type& interface_ )
  : base_type( interface_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_ack_finder_p(0)
  {}

    loki_inout( const char* name_, inout_if_type& interface_ )
  : base_type( name_, interface_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_ack_finder_p(0)
  {}

    explicit loki_inout( inout_port_type& parent_ )
  : base_type( parent_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_ack_finder_p(0)
  {}

    loki_inout( const char* name_, inout_port_type& parent_ )
  : base_type( name_, parent_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_ack_finder_p(0)
  {}

    loki_inout( this_type& parent_ )
  : base_type( parent_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_ack_finder_p(0)
  {}

    loki_inout( const char* name_, this_type& parent_ )
  : base_type( name_, parent_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_ack_finder_p(0)
  {}


    // destructor

    virtual ~loki_inout();


    // interface access shortcut methods

    // get the default event

    const sc_event& default_event() const
  { return (*this)->default_event(); }


    // get the value changed event

    const sc_event& value_changed_event() const
  { return (*this)->value_changed_event(); }


    // read the current value

    const data_type& read() const
  { return (*this)->read(); }

    operator const data_type& () const
  { return (*this)->read(); }


    // was there a value changed event?

    bool event() const
  { return (*this)->event(); }


    bool valid() const {
      return (*this)->valid();
    }
    void ack() {
      (*this)->ack();
    }
    const sc_event& ack_event() const {
      return (*this)->ack_event();
    }


    // write the new value

    void write( const data_type& value_ )
  { (*this)->write( value_ ); }

    this_type& operator = ( const data_type& value_ )
  { (*this)->write( value_ ); return *this; }

    this_type& operator = ( const in_if_type& interface_ )
  { (*this)->write( interface_.read() ); return *this; }

    this_type& operator = ( const in_port_type& port_ )
  { (*this)->write( port_->read() ); return *this; }

    this_type& operator = ( const inout_port_type& port_ )
  { (*this)->write( port_->read() ); return *this; }

    this_type& operator = ( const this_type& port_ )
  { (*this)->write( port_->read() ); return *this; }


    // set initial value (can also be called when port is not bound yet)

    void initialize( const data_type& value_ );

    void initialize( const in_if_type& interface_ )
  { initialize( interface_.read() ); }


    // called when elaboration is done
    /*  WHEN DEFINING THIS METHOD IN A DERIVED CLASS, */
    /*  MAKE SURE THAT THIS METHOD IS CALLED AS WELL. */

    virtual void end_of_elaboration();


    // (other) event finder method(s)

    sc_event_finder& value_changed() const
    {
        if ( !m_change_finder_p )
  {
      m_change_finder_p = new sc_event_finder_t<in_if_type>(
          *this, &in_if_type::value_changed_event );
  }
  return *m_change_finder_p;
    }

    sc_event_finder& ack_finder() const {
      if(!m_ack_finder_p) {
        m_ack_finder_p = new sc_event_finder_t<if_type>(
            *this, &if_type::ack_event);
      }
      return *m_ack_finder_p;
    }

    virtual const char* kind() const
        { return "loki_inout"; }

protected:

    data_type* m_init_val;

public:

    // called by sc_trace
    void add_trace_internal( sc_trace_file*, const std::string& ) const;

    void add_trace( sc_trace_file*, const std::string& ) const;

protected:

    void remove_traces() const;

    mutable sc_trace_params_vec* m_traces;

private:
  mutable sc_event_finder* m_change_finder_p;
  mutable sc_event_finder* m_ack_finder_p;

private:

    // disabled
    loki_inout( const this_type& );

#ifdef __GNUC__
    // Needed to circumvent a problem in the g++-2.95.2 compiler:
    // This unused variable forces the compiler to instantiate
    // an object of T template so an implicit conversion from
    // read() to a C++ intrinsic data type will work.
    static data_type dummy;
#endif
};

template<typename T>
::std::ostream& operator << ( ::std::ostream& os, const loki_inout<T>& a )
{
    return os << a->read();
}

// IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII


// destructor

template <class T>
inline
loki_inout<T>::~loki_inout()
{
    if ( m_change_finder_p ) delete m_change_finder_p;

    // FIXME: Not sure why this segfaults here...
//    if ( m_ack_finder_p ) delete m_ack_finder_p;

    if( m_init_val != 0 ) {
  delete m_init_val;
    }
    remove_traces();
}


// set initial value (can also be called when port is not bound yet)

template <class T>
inline
void
loki_inout<T>::initialize( const data_type& value_ )
{
    inout_if_type* iface = dynamic_cast<inout_if_type*>( this->get_interface() );
    if( iface != 0 ) {
  iface->write( value_ );
    } else {
  if( m_init_val == 0 ) {
      m_init_val = new data_type;
  }
  *m_init_val = value_;
    }
}


// called when elaboration is done

template <class T>
inline
void
loki_inout<T>::end_of_elaboration()
{
    if( m_init_val != 0 ) {
  write( *m_init_val );
  delete m_init_val;
  m_init_val = 0;
    }
    if( m_traces != 0 ) {
  for( int i = 0; i < (int)m_traces->size(); ++ i ) {
      sc_trace_params* p = (*m_traces)[i];
      in_if_type* iface = dynamic_cast<in_if_type*>( this->get_interface() );
      sc_trace( p->tf, iface->read(), p->name );
  }
  remove_traces();
    }
}


// called by sc_trace

template <class T>
inline
void
loki_inout<T>::add_trace_internal( sc_trace_file* tf_, const std::string& name_)
const
{
    if( tf_ != 0 ) {
      if( m_traces == 0 ) {
          m_traces = new sc_trace_params_vec;
      }
      m_traces->push_back( new sc_trace_params( tf_, name_ ) );
    }
}

template <class T>
inline
void
loki_inout<T>::add_trace( sc_trace_file* tf_, const std::string& name_) const
{
    sc_core::sc_deprecated_add_trace();
    add_trace_internal(tf_, name_);
}

template <class T>
inline
void
loki_inout<T>::remove_traces() const
{
    if( m_traces != 0 ) {
    for( int i = m_traces->size() - 1; i >= 0; -- i ) {
          delete (*m_traces)[i];
    }
    delete m_traces;
    m_traces = 0;
    }
}


// ----------------------------------------------------------------------------
//  CLASS : loki_inout<bool>
//
//  Specialization of loki_inout<T> for type bool.
// ----------------------------------------------------------------------------

template <>
class loki_inout<bool> :
    public sc_port<loki_signal_inout_if<bool>,1,SC_ONE_OR_MORE_BOUND>
{
public:

    // typedefs

    typedef bool                                       data_type;

    typedef loki_signal_inout_if<data_type>              if_type;
    typedef sc_port<if_type,1,SC_ONE_OR_MORE_BOUND>    base_type;
    typedef loki_inout<data_type>                        this_type;

    typedef loki_signal_in_if<data_type>                 in_if_type;
    typedef sc_port<in_if_type,1,SC_ONE_OR_MORE_BOUND> in_port_type;
    typedef if_type                                    inout_if_type;
    typedef base_type                                  inout_port_type;

public:

    // constructors

    loki_inout()
  : base_type(), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    explicit loki_inout( const char* name_ )
  : base_type( name_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    explicit loki_inout( inout_if_type& interface_ )
  : base_type( interface_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    loki_inout( const char* name_, inout_if_type& interface_ )
  : base_type( name_, interface_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    explicit loki_inout( inout_port_type& parent_ )
  : base_type( parent_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    loki_inout( const char* name_, inout_port_type& parent_ )
  : base_type( name_, parent_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    loki_inout( this_type& parent_ )
  : base_type( parent_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    loki_inout( const char* name_, this_type& parent_ )
  : base_type( name_, parent_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}


    // destructor

    virtual ~loki_inout();


    // interface access shortcut methods

    // get the default event

    const sc_event& default_event() const
  { return (*this)->default_event(); }


    // get the value changed event

    const sc_event& value_changed_event() const
  { return (*this)->value_changed_event(); }

    // get the positive edge event

    const sc_event& posedge_event() const
  { return (*this)->posedge_event(); }

    // get the negative edge event

    const sc_event& negedge_event() const
  { return (*this)->negedge_event(); }


    // read the current value

    const data_type& read() const
  { return (*this)->read(); }

    operator const data_type& () const
  { return (*this)->read(); }


    // use for positive edge sensitivity

    sc_event_finder& pos() const
    {
        if ( !m_pos_finder_p )
  {
      m_pos_finder_p = new sc_event_finder_t<in_if_type>(
          *this, &in_if_type::posedge_event );
  }
  return *m_pos_finder_p;
    }

    // use for negative edge sensitivity

    sc_event_finder& neg() const
    {
        if ( !m_neg_finder_p )
  {
      m_neg_finder_p = new sc_event_finder_t<in_if_type>(
          *this, &in_if_type::negedge_event );
  }
  return *m_neg_finder_p;
    }


    // was there a value changed event?

    bool event() const
  { return (*this)->event(); }

    // was there a positive edge event?

    bool posedge() const
        { return (*this)->posedge(); }

    // was there a negative edge event?

    bool negedge() const
        { return (*this)->negedge(); }

    // write the new value

    void write( const data_type& value_ )
  { (*this)->write( value_ ); }

    this_type& operator = ( const data_type& value_ )
  { (*this)->write( value_ ); return *this; }

    this_type& operator = ( const in_if_type& interface_ )
  { (*this)->write( interface_.read() ); return *this; }

    this_type& operator = ( const in_port_type& port_ )
  { (*this)->write( port_->read() ); return *this; }

    this_type& operator = ( const inout_port_type& port_ )
  { (*this)->write( port_->read() ); return *this; }

    this_type& operator = ( const this_type& port_ )
  { (*this)->write( port_->read() ); return *this; }


    // set initial value (can also be called when port is not bound yet)

    void initialize( const data_type& value_ );

    void initialize( const in_if_type& interface_ )
  { initialize( interface_.read() ); }


    // called when elaboration is done
    /*  WHEN DEFINING THIS METHOD IN A DERIVED CLASS, */
    /*  MAKE SURE THAT THIS METHOD IS CALLED AS WELL. */

    virtual void end_of_elaboration();


    // (other) event finder method(s)

    sc_event_finder& value_changed() const
    {
        if ( !m_change_finder_p )
  {
      m_change_finder_p = new sc_event_finder_t<in_if_type>(
          *this, &in_if_type::value_changed_event );
  }
  return *m_change_finder_p;
    }

    virtual const char* kind() const
        { return "loki_inout"; }

protected:

    data_type* m_init_val;

public:

    // called by sc_trace
    void add_trace_internal( sc_trace_file*, const std::string& ) const;

    void add_trace( sc_trace_file*, const std::string& ) const;

protected:

    void remove_traces() const;

    mutable sc_trace_params_vec* m_traces;

private:
  mutable sc_event_finder* m_change_finder_p;
  mutable sc_event_finder* m_neg_finder_p;
  mutable sc_event_finder* m_pos_finder_p;

private:

    // disabled
    loki_inout( const this_type& );

#ifdef __GNUC__
    // Needed to circumvent a problem in the g++-2.95.2 compiler:
    // This unused variable forces the compiler to instantiate
    // an object of T template so an implicit conversion from
    // read() to a C++ intrinsic data type will work.
    static data_type dummy;
#endif
};


// ----------------------------------------------------------------------------
//  CLASS : loki_inout<sc_dt::sc_logic>
//
//  Specialization of loki_inout<T> for type sc_dt::sc_logic.
// ----------------------------------------------------------------------------

template <>
class loki_inout<sc_dt::sc_logic>
: public sc_port<loki_signal_inout_if<sc_dt::sc_logic>,1,SC_ONE_OR_MORE_BOUND>
{
public:

    // typedefs

    typedef sc_dt::sc_logic                            data_type;

    typedef loki_signal_inout_if<data_type>              if_type;
    typedef sc_port<if_type,1,SC_ONE_OR_MORE_BOUND>    base_type;
    typedef loki_inout<data_type>                        this_type;

    typedef loki_signal_in_if<data_type>                 in_if_type;
    typedef sc_port<in_if_type,1,SC_ONE_OR_MORE_BOUND> in_port_type;
    typedef if_type                                    inout_if_type;
    typedef base_type                                  inout_port_type;

public:

    // constructors

    loki_inout()
  : base_type(), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    explicit loki_inout( const char* name_ )
  : base_type( name_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    explicit loki_inout( inout_if_type& interface_ )
  : base_type( interface_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    loki_inout( const char* name_, inout_if_type& interface_ )
  : base_type( name_, interface_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    explicit loki_inout( inout_port_type& parent_ )
  : base_type( parent_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    loki_inout( const char* name_, inout_port_type& parent_ )
  : base_type( name_, parent_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    loki_inout( this_type& parent_ )
  : base_type( parent_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}

    loki_inout( const char* name_, this_type& parent_ )
  : base_type( name_, parent_ ), m_init_val( 0 ), m_traces( 0 ),
    m_change_finder_p(0), m_neg_finder_p(0), m_pos_finder_p(0)
  {}


    // destructor

    virtual ~loki_inout();


    // interface access shortcut methods

    // get the default event

    const sc_event& default_event() const
  { return (*this)->default_event(); }


    // get the value changed event

    const sc_event& value_changed_event() const
  { return (*this)->value_changed_event(); }

    // get the positive edge event

    const sc_event& posedge_event() const
  { return (*this)->posedge_event(); }

    // get the negative edge event

    const sc_event& negedge_event() const
  { return (*this)->negedge_event(); }


    // read the current value

    const data_type& read() const
  { return (*this)->read(); }

    operator const data_type& () const
  { return (*this)->read(); }


    // use for positive edge sensitivity

    sc_event_finder& pos() const
    {
        if ( !m_pos_finder_p )
  {
      m_pos_finder_p = new sc_event_finder_t<in_if_type>(
          *this, &in_if_type::posedge_event );
  }
  return *m_pos_finder_p;
    }

    // use for negative edge sensitivity

    sc_event_finder& neg() const
    {
        if ( !m_neg_finder_p )
  {
      m_neg_finder_p = new sc_event_finder_t<in_if_type>(
          *this, &in_if_type::negedge_event );
  }
  return *m_neg_finder_p;
    }


    // was there a value changed event?

    bool event() const
  { return (*this)->event(); }

    // was there a positive edge event?

    bool posedge() const
        { return (*this)->posedge(); }

    // was there a negative edge event?

    bool negedge() const
        { return (*this)->negedge(); }

    // write the new value

    void write( const data_type& value_ )
  { (*this)->write( value_ ); }

    this_type& operator = ( const data_type& value_ )
  { (*this)->write( value_ ); return *this; }

    this_type& operator = ( const in_if_type& interface_ )
  { (*this)->write( interface_.read() ); return *this; }

    this_type& operator = ( const in_port_type& port_ )
  { (*this)->write( port_->read() ); return *this; }

    this_type& operator = ( const inout_port_type& port_ )
  { (*this)->write( port_->read() ); return *this; }

    this_type& operator = ( const this_type& port_ )
  { (*this)->write( port_->read() ); return *this; }


    // set initial value (can also be called when port is not bound yet)

    void initialize( const data_type& value_ );

    void initialize( const in_if_type& interface_ )
  { initialize( interface_.read() ); }


    // called when elaboration is done
    /*  WHEN DEFINING THIS METHOD IN A DERIVED CLASS, */
    /*  MAKE SURE THAT THIS METHOD IS CALLED AS WELL. */

    virtual void end_of_elaboration();


    // (other) event finder method(s)

    sc_event_finder& value_changed() const
    {
        if ( !m_change_finder_p )
  {
      m_change_finder_p = new sc_event_finder_t<in_if_type>(
          *this, &in_if_type::value_changed_event );
  }
        return *m_change_finder_p;
    }

    virtual const char* kind() const
        { return "loki_inout"; }

protected:

    data_type* m_init_val;

public:

    // called by sc_trace
    void add_trace_internal( sc_trace_file*, const std::string& ) const;

    void add_trace( sc_trace_file*, const std::string& ) const;

protected:

    void remove_traces() const;

    mutable sc_trace_params_vec* m_traces;

private:
  mutable sc_event_finder* m_change_finder_p;
  mutable sc_event_finder* m_neg_finder_p;
  mutable sc_event_finder* m_pos_finder_p;

private:

    // disabled
    loki_inout( const this_type& );

#ifdef __GNUC__
    // Needed to circumvent a problem in the g++-2.95.2 compiler:
    // This unused variable forces the compiler to instantiate
    // an object of T template so an implicit conversion from
    // read() to a C++ intrinsic data type will work.
    static data_type dummy;
#endif
};


// ----------------------------------------------------------------------------
//  CLASS : loki_out<T>
//
//  The loki_signal<T> output port class.
// ----------------------------------------------------------------------------

// loki_out can also read from its port, hence no difference with loki_inout.
// For debugging reasons, a class is provided instead of a define.

template <class T>
class loki_out
: public loki_inout<T>
{
public:

    // typedefs

    typedef T                                   data_type;

    typedef loki_out<data_type>                   this_type;
    typedef loki_inout<data_type>                 base_type;

    typedef typename base_type::in_if_type      in_if_type;
    typedef typename base_type::in_port_type    in_port_type;
    typedef typename base_type::inout_if_type   inout_if_type;
    typedef typename base_type::inout_port_type inout_port_type;

public:

    // constructors

    loki_out()
  : base_type()
  {}

    explicit loki_out( const char* name_ )
  : base_type( name_ )
  {}

    explicit loki_out( inout_if_type& interface_ )
  : base_type( interface_ )
  {}

    loki_out( const char* name_, inout_if_type& interface_ )
  : base_type( name_, interface_ )
  {}

    explicit loki_out( inout_port_type& parent_ )
  : base_type( parent_ )
  {}

    loki_out( const char* name_, inout_port_type& parent_ )
  : base_type( name_, parent_ )
  {}

    loki_out( this_type& parent_ )
  : base_type( parent_ )
  {}

    loki_out( const char* name_, this_type& parent_ )
  : base_type( name_, parent_ )
  {}


    // destructor (does nothing)

    virtual ~loki_out()
  {}


    // write the new value

    this_type& operator = ( const data_type& value_ )
  { (*this)->write( value_ ); return *this; }

    this_type& operator = ( const in_if_type& interface_ )
  { (*this)->write( interface_.read() ); return *this; }

    this_type& operator = ( const in_port_type& port_ )
  { (*this)->write( port_->read() ); return *this; }

    this_type& operator = ( const inout_port_type& port_ )
  { (*this)->write( port_->read() ); return *this; }

    this_type& operator = ( const this_type& port_ )
  { (*this)->write( port_->read() ); return *this; }

    virtual const char* kind() const
        { return "loki_out"; }

private:

    // disabled
    loki_out( const this_type& );
};


// IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII


// ----------------------------------------------------------------------------
//  FUNCTION : sc_trace
// ----------------------------------------------------------------------------

template <class T>
inline
void
sc_trace(sc_trace_file* tf, const loki_in<T>& port, const std::string& name)
{
    const loki_signal_in_if<T>* iface = 0;
    if (sc_core::sc_get_curr_simcontext()->elaboration_done() )
    {
  iface = dynamic_cast<const loki_signal_in_if<T>*>( port.get_interface() );
    }

    if ( iface )
  sc_trace( tf, iface->read(), name );
    else
  port.add_trace_internal( tf, name );
}

template <class T>
inline
void
sc_trace( sc_trace_file* tf, const loki_inout<T>& port,
    const std::string& name )
{
    const loki_signal_in_if<T>* iface = 0;
    if (sc_core::sc_get_curr_simcontext()->elaboration_done() )
    {
  iface = dynamic_cast<const loki_signal_in_if<T>*>( port.get_interface() );
    }

    if ( iface )
  sc_trace( tf, iface->read(), name );
    else
  port.add_trace_internal( tf, name );
}


#endif /* LOKI_PORTS_H_ */
