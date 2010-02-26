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

#include "Utility/Parameters.h"

using sc_core::sc_in;
using sc_core::sc_out;
using sc_core::sc_signal;
using sc_core::sc_buffer;   // Like an sc_signal, but every write makes an event
using sc_core::sc_module_name;

using std::cout;
using std::endl;

SC_MODULE (Component) {

public:
/* Local state */
  int id;

/* Constructors and destructors */
  Component(sc_module_name& name);
  Component(sc_module_name& name, int ID);
  ~Component();

  // DO NOT MAKE A COPY CONSTRUCTOR. SYSTEMC MODULES CANNOT BE COPIED PROPERLY

private:
  static std::string makeName(sc_module_name& name, int ID);

};

#endif /* COMPONENT_H_ */
