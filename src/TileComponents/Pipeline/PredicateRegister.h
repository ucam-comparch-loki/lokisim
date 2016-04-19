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

//============================================================================//
// Methods
//============================================================================//

public:

  // Get the current value.
  bool read() const;

  // Write a new value to the register.
  void write(bool val);

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  PredicateRegister(const sc_module_name& name);

//============================================================================//
// Local state
//============================================================================//

private:

  bool predicate;

};

#endif /* PREDICATEREGISTER_H_ */
