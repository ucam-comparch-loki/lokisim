/*
 * PredicateRegister.h
 *
 * A register containing a single boolean value.
 *
 *  Created on: 22 Mar 2010
 *      Author: db434
 */

#ifndef PREDICATEREGISTER_H_
#define PREDICATEREGISTER_H_

#include "../../Component.h"

class PredicateRegister: public Component {

//==============================//
// Ports
//==============================//

public:

  // The value to write to the predicate register.
  sc_in<bool>  write;

  // Return the result of a read.
  sc_out<bool> output;

//==============================//
// Methods
//==============================//

  // Get the current value.
  bool getVal() const;

private:

  // Write a new value to the register.
  void writeVal();

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(PredicateRegister);
  PredicateRegister(sc_module_name name);
  virtual ~PredicateRegister();

//==============================//
// Local state
//==============================//

private:

  bool predicate;

};

#endif /* PREDICATEREGISTER_H_ */
