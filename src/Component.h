/*
 * Component.h
 *
 * The base class for all modules of the Loki chip. Allows provision of functions
 * such as area() and print() which can then be used by all components.
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef COMPONENT_H_
#define COMPONENT_H_

#include "systemc"
#include "Typedefs.h"

#include "Utility/Instrumentation.h"
#include "Utility/Parameters.h"

using sc_core::sc_in;
using sc_core::sc_out;
using sc_core::sc_signal;
using sc_core::sc_buffer;   // Like an sc_signal, but every write makes an event
using sc_core::sc_module_name;

using std::cout;
using std::cerr;
using std::endl;

SC_MODULE (Component) {

//==============================//
// Local state
//==============================//

public:

  ComponentID id; // Would like to be const, but seems hard to do

//==============================//
// Constructors and destructors
//==============================//

public:

  Component(sc_module_name& name);
  Component(sc_module_name& name, ComponentID ID);
  ~Component();

  // DO NOT MAKE A COPY CONSTRUCTOR. SYSTEMC MODULES SHOULD NOT BE COPIED.

//==============================//
// Methods
//==============================//

public:

  // An estimate of the component's area in square micrometres.
  virtual double area()   const;

  // An estimate of the energy consumed by the component in picojoules.
  virtual double energy() const;

private:

  // Generate a unique name for this component, using its ID.
  static std::string makeName(sc_module_name& name, ComponentID ID);

};

#endif /* COMPONENT_H_ */
