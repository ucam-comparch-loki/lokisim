/*
 * Multiplexor.h
 *
 * Base class of all multiplexors. SystemC does not allow variable numbers of
 * inputs or outputs, so a separate module must be made for each case. Each
 * subclass represents a multiplexor with a different number of inputs.
 *
 * This approach was considered appropriate in this case (but not others),
 * since all of the inputs come from different places, and it would be awkward
 * to combine them.
 *
 * Any signal connecting to the select port must (?) be an sc_buffer, so it
 * triggers the multiplexor every time it is written, instead of only when the
 * select value changes.
 *
 * Since this class is templated, all of the implementation must go in the
 * header file.
 *
 *  Created on: 20 Jan 2010
 *      Author: db434
 */

#ifndef MULTIPLEXOR_H_
#define MULTIPLEXOR_H_

#include "../Component.h"

template<class T>
class Multiplexor: public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<short> select;
  sc_out<T>    result;

  // Inputs to be declared in subclasses

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Multiplexor);
  Multiplexor(sc_module_name name) : Component(name) {

    SC_METHOD(doOp);
    sensitive << select;
    dont_initialize();

  }

  virtual ~Multiplexor() {

  }

//==============================//
// Methods
//==============================//

protected:

  virtual void doOp() = 0;

};

#endif /* MULTIPLEXOR_H_ */
