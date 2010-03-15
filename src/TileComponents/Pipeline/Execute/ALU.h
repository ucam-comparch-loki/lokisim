/*
 * ALU.h
 *
 * Arithmetic Logic Unit.
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#ifndef ALU_H_
#define ALU_H_

#include "../../../Component.h"
#include "../../../Datatype/Data.h"

class ALU: public Component {

public:

/* Ports */

  // The two input values.
  sc_in<Data>   in1, in2;

  // The operation the ALU should carry out.
  sc_in<short>  select;

  // Signal telling whether or not the computation should be conditional on
  // some value of predicate register.
  sc_in<short>  predicate;

  // Tells whether the result of this computation should be used to set the
  // predicate register.
  sc_in<bool>   setPredicate;

  // The result of the computation.
  sc_out<Data>  out;

/* Constructors and destructors */
  SC_HAS_PROCESS(ALU);
  ALU(sc_core::sc_module_name name);
  virtual ~ALU();

private:
/* Methods */
  void doOp();

/* Local state */
  bool predicateVal;

};

#endif /* ALU_H_ */
