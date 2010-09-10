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

  // The memory operation we are performing.
  sc_out<short>       memoryOp;

  // The register to write the result back to.
  sc_out<short>       writeAddr;

  // The address of the indirect register we want to write the result to.
  sc_out<short>       indWrite;

  // The remote channel to send the data/instruction to.
  sc_out<short>       rChannel;

  // Stall the pipeline until this output channel is empty.
  sc_out<short>       waitOnChannel;

  // The current value of the predicate register.
  sc_in<bool>         predicate;

  // Tell the ALU whether the instruction is dependent on the value of the
  // predicate register.
  sc_out<short>       usePredicate;

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

  // Signals whether the same packet should be executed repeatedly.
  sc_out<bool>        persistent;

  // The address of the instruction packet we want to fetch.
  sc_out<Address>     toFetchLogic;

  // The immediate we want to be padded to 32 bits.
  sc_out<Data>        toSignExtend;

  // Tell the pipeline to stall.
  sc_out<bool>        stall;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Decoder);
  Decoder(sc_module_name name, int id);
  virtual ~Decoder();

//==============================//
// Methods
//==============================//

private:

  // The main decoding method. Extracts information from the instruction, and
  // determines what to do with it.
  void decodeInstruction();

  // Determine whether to read the first operand from the receive channel-end
  // table, the ALU or the register file.
  void setOperand1(short operation, int operand);

  // Determine whether to read the second operand from the receive channel-end
  // table, the ALU, the register file, or the sign extender.
  void setOperand2(short operation, int operand, int immediate);

  // Send the index of the destination register.
  void setDestination(int destination);

  // Write operations take two cycles since there are two flits to send. This
  // method sends the second part.
  void completeWrite();

  // Determine whether the current instruction should be executed, based on its
  // predicate bits, and the contents of the predicate register.
  bool shouldExecute(short predBits);

  // Determines whether a read from the specified register will have to be
  // converted to data forwarding from the ALU, as the register has not yet
  // been written to.
  bool readALUOutput(short reg);

//==============================//
// Local state
//==============================//

public:

  // The possible sources of ALU operands.
  enum Source {RCET, REGISTERS, ALU, SIGN_EXTEND};

private:

  // The remote channel we are fetching from (set using SETFETCHCH).
  // Note: this MUST be set to a value before use: -1 confuses things.
  // Move this into fetch logic?
  int  fetchChannel;

  // The remote channel we are sending instructions to.
  int  sendChannel;

  // The destination register of the previous operation. Used to determine
  // whether data should come from the register or directly from the ALU.
  int  regLastWritten;

  // Tells whether or not we are in remote execution mode.
  bool remoteExecute;

  // Tells whether we have started a two-cycle store operation.
  bool currentlyWriting;

};

#endif /* DECODER_H_ */
