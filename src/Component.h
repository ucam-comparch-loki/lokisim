/*
 * Component.h
 *
 * The base class for all modules of the Loki chip. Allows provision of functions
 * such as area() and printStatus() which can then be used by all components.
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef COMPONENT_H_
#define COMPONENT_H_

#include "systemc"

#include "Parameters.h"

using sc_core::sc_in;
using sc_core::sc_out;
using sc_core::sc_signal;
using sc_core::sc_buffer;   // Like an sc_signal, but every write makes an event

SC_MODULE (Component) {

/* Local state */
  int id;

public:
/* Constructors and destructors */
  SC_CTOR(Component);
  Component(sc_core::sc_module_name name, int ID);
  Component(const Component& other);
  ~Component();

};

#endif /* COMPONENT_H_ */
