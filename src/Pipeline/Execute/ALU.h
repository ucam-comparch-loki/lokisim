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

#include "../../Component.h"
#include "../../Datatype/Data.h"

class ALU: public Component {

/* Methods */
  void doOp();

public:
/* Ports */
  sc_in<short> select;
  sc_in<Data> in1, in2;
  sc_out<Data> out;

/* Constructors and destructors */
  SC_HAS_PROCESS(ALU);
  ALU(sc_core::sc_module_name name, int ID);
  virtual ~ALU();

};

#endif /* ALU_H_ */
