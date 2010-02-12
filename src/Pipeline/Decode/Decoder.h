/*
 * Decoder.h
 *
 * Component responsible for interpreting the instruction and distributing
 * relevant information to other components. This information includes
 * where to find data, where to write data, immediate values, and the
 * operation to perform.
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#ifndef DECODER_H_
#define DECODER_H_

#include "../../Component.h"
#include "../../Datatype/Instruction.h"
#include "../../Datatype/Address.h"
#include "../../Datatype/Data.h"

class Decoder: public Component {

public:
/* Ports */
  sc_in<Instruction> instructionIn;
  sc_out<Instruction> instructionOut;
  sc_out<short> toRCET1, toRCET2;
  sc_out<short> regAddr1, regAddr2, indWrite, writeAddr, rChannel, predicate;
  sc_out<bool> isIndirectRead, newRChannel;
  sc_out<short> operation, op1Select, op2Select;
  sc_out<Address> toFetchLogic;
  sc_out<Data> toSignExtend;

/* Constructors and destructors */
  SC_HAS_PROCESS(Decoder);
  Decoder(sc_core::sc_module_name name, int ID);
  virtual ~Decoder();

private:
/* Methods */
  void doOp();

/* Local state */
  int fetchChannel;
  int regLastWritten;

};

#endif /* DECODER_H_ */
