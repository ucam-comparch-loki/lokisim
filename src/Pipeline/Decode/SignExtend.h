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

#include "../../Component.h"
#include "../../Datatype/Data.h"

class SignExtend: public Component {

/* Methods */
  void doOp();

public:
/* Ports */
  sc_in<Data> input;
  sc_out<Data> output;

/* Constructors and destructors */
  SC_HAS_PROCESS(SignExtend);
  SignExtend(sc_core::sc_module_name name, int ID);
  virtual ~SignExtend();

};

#endif /* SIGNEXTEND_H_ */
