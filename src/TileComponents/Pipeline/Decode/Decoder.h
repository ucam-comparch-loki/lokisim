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

#include "../../../Component.h"
#include "../../../Datatype/Instruction.h"
#include "../../../Datatype/Address.h"
#include "../../../Datatype/Data.h"

class Decoder: public Component {

//==============================//
// Ports
//==============================//

public:

  // The instruction to decode.
  sc_in<Instruction>  instructionIn;

  // The instruction to be sent to another cluster.
  sc_out<Instruction> instructionOut;

  // The registers to retrieve data from.
  sc_out<short>       regAddr1, regAddr2;

  // Tells the register file whether or not the second read address is indirect.
  sc_out<bool>        isIndirectRead;

  // The channel-ends to retrieve data from.
  sc_out<short>       toRCET1, toRCET2;

  // The operation for the receive channel-end table to perform.
  sc_out<short>       channelOp;

  // The register to write the result back to.
  sc_out<short>       writeAddr;

  // The address of the indirect register we want to write the result to.
  sc_out<short>       indWrite;

  // The remote channel to send the data/instruction to.
  sc_out<short>       rChannel;

  // Stall the pipeline until this output channel is empty.
  sc_out<short>       waitOnChannel;

  // Tell the ALU whether an instruction should always execute, or be
  // conditional on the predicate register being true or false.
  sc_out<short>       predicate;

  // Tell the ALU whether it should use the result of this instruction to
  // set the predicate register.
  sc_out<bool>        setPredicate;

  // The operation the ALU is to carry out.
  sc_out<short>       operation;

  // Choose where ALU's operands come from by selecting appropriate
  // multiplexor inputs.
  sc_out<short>       op1Select, op2Select;

  // The amount we want to jump by in the instruction packet cache.
  sc_out<short>       jumpOffset;

  // The address of the instruction packet we want to fetch.
  sc_out<Address>     toFetchLogic;

  // The immediate we want to be padded to 32 bits.
  sc_out<Data>        toSignExtend;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Decoder);
  Decoder(sc_module_name name);
  virtual ~Decoder();

//==============================//
// Methods
//==============================//

private:

  void decodeInstruction();

//==============================//
// Local state
//==============================//

private:

  // The possible sources of ALU operands.
  enum Source {RCET, REGISTERS, ALU, SIGN_EXTEND};

  // The remote channel we are fetching from (set using SETFETCHCH).
  int  fetchChannel;

  // The remote channel we are sending instructions to.
  int  sendChannel;

  // The destination register of the previous operation. Used to determine
  // whether data should come from the register or directly from the ALU.
  int  regLastWritten;

  // Tells whether or not we are in remote execution mode.
  bool remoteExecute;

};

#endif /* DECODER_H_ */
