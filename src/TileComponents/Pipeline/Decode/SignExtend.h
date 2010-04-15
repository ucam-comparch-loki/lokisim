/*
 * SignExtend.h
 *
 * Component which pads out immediate values to 32 bits, whilst ensuring
 * that the sign of the value is unchanged.
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#ifndef SIGNEXTEND_H_
#define SIGNEXTEND_H_

#include "../../../Component.h"
#include "../../../Datatype/Data.h"

class SignExtend: public Component {

//==============================//
// Ports
//==============================//

public:

  // The input value.
  sc_in<Data>   input;

  // The output value, padded to 32 bits.
  sc_out<Data>  output;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(SignExtend);
  SignExtend(sc_module_name name);
  virtual ~SignExtend();

//==============================//
// Methods
//==============================//

private:

  // Pad the received value out to 32 bits, preserving the sign.
  void doOp();

};

#endif /* SIGNEXTEND_H_ */
