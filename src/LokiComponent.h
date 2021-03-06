/*
 * LokiComponent.h
 *
 * The base class for all modules of the Loki chip.
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef SRC_LOKICOMPONENT_H_
#define SRC_LOKICOMPONENT_H_

#include "systemc"
#include <vector>
#include "Datatype/Identifier.h"
#include "Types.h"

#include "Utility/Logging.h"

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

SC_MODULE (LokiComponent) {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  LokiComponent(const sc_module_name& name);

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

#endif /* SRC_LOKICOMPONENT_H_ */
