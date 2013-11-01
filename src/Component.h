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
#include <vector>
#include "Typedefs.h"

#include "Datatype/ComponentID.h"

#include "Utility/LokiVector.h"
#include "Utility/LokiVector2D.h"
#include "Utility/Parameters.h"

using sc_core::sc_in;
using sc_core::sc_inout;
using sc_core::sc_out;
using sc_core::sc_signal;
using sc_core::sc_buffer;   // Like an sc_signal, but every write makes an event

using sc_core::sc_event;
using sc_core::sc_module_name;
using sc_core::sc_gen_unique_name;

using std::cout;
using std::cerr;
using std::endl;

using std::vector;

SC_MODULE (Component) {

//==============================//
// Local state
//==============================//

public:

  const ComponentID id;

//==============================//
// Constructors and destructors
//==============================//

public:

  Component(const sc_module_name& name);
  Component(const sc_module_name& name, const ComponentID& ID);

  // DO NOT MAKE A COPY CONSTRUCTOR. SYSTEMC MODULES SHOULD NOT BE COPIED.

};

// Macro to hide much of the boilerplate code required to generate a dynamic
// process.
//   sensitive_to = event, port, signal, etc
//   function = fully qualified function
//   argument = function's argument
//   initialise = boolean telling whether function should be executed immediately
// Note that SC_INCLUDE_DYNAMIC_PROCESSES must be defined wherever this macro
// is used.
#define SPAWN_METHOD(sensitive_to, function, argument, initialise) {\
    sc_core::sc_spawn_options options; \
    options.spawn_method();     /* Want an efficient method, not a thread */ \
    options.set_sensitivity(&(sensitive_to)); /* Sensitive to this event */ \
\
    if(!initialise)\
      options.dont_initialize();\
\
    /* Create the method. */ \
    sc_spawn(sc_bind(&function, this, argument), 0, &options);\
}

#endif /* COMPONENT_H_ */
