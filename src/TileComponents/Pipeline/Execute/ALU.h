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

//==============================//
// Ports
//==============================//

public:

  // The two input values.
  sc_in<Data>   in1, in2;

  // The operation the ALU should carry out.
  sc_in<short>  operation;

  // Tells whether the execution of this operation is dependent on the
  // predicate register.
  sc_in<short>  usePredicate;

  // Tells whether the result of this computation should be used to set the
  // predicate register.
  sc_in<bool>   setPredicate;

  // Write the new predicate value to the register.
  sc_out<bool>  predicate;

  // The result of the computation.
  sc_out<Data>  out;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(ALU);
  ALU(sc_module_name name);
  virtual ~ALU();

//==============================//
// Methods
//==============================//

private:

  void doOp();
  void setPred(bool val);

  // Determine whether the current instruction should be executed, based on its
  // predicate bits, and the contents of the predicate register.
  bool shouldExecute(short predBits);

//==============================//
// Methods
//==============================//

private:

  bool pred;  // Testing having predicate register in ALU

};

#endif /* ALU_H_ */
