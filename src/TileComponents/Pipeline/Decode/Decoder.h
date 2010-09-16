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

class Address;
class DecodedInst;
class DecodeStage;
class Instruction;

class Decoder: public Component {

//==============================//
// Methods
//==============================//

public:

  // Extract information from the encoded instruction and determine what the
  // operands are to be. Stores result in DecodedInst. Returns whether there
  // is useful output this cycle -- the first cycle of multi-cycle operations
  // will return false.
  bool decodeInstruction(Instruction i, DecodedInst& dec);

  // Returns whether the decoder is ready to accept a new instruction.
  bool ready();

private:

  // Determine whether to read the first operand from the receive channel-end
  // table, the ALU or the register file.
  void setOperand1(DecodedInst& dec);

  // Determine whether to read the second operand from the receive channel-end
  // table, the ALU, the register file, or the sign extender.
  void setOperand2(DecodedInst& dec);

  // Similar methods, giving more control to the user. Some instructions
  // use the first register value (in the destination position) as an
  // argument.
  void setOperand1ToValue(DecodedInst& dec, int32_t reg);
  void setOperand2ToValue(DecodedInst& dec, int32_t reg, int32_t immed);

  // Read a register value.
  int32_t readRegs(uint8_t index, bool indirect = false);

  // Read a value from an input buffer.
  int32_t readRCET(uint8_t index);

  // Ensure that the instruction packet from the given address is in the
  // instruction packet cache. Nothing will be done if it is already there.
  void    fetch(Address a);

  // Write operations take two cycles since there are two flits to send. This
  // method sends the second part.
  bool completeWrite(Instruction i, DecodedInst& dec);

  // Determine whether the current instruction should be executed, based on its
  // predicate bits, and the contents of the predicate register.
  bool shouldExecute(short predBits);

  // Determines whether a read from the specified register will have to be
  // converted to data forwarding from the ALU, as the register has not yet
  // been written to.
  bool readALUOutput(short reg);

  DecodeStage* parent() const;

//==============================//
// Constructors and destructors
//==============================//

public:

  Decoder(sc_module_name name, int id);
  virtual ~Decoder();

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

  // Tells whether we are blocked (probably trying to read from an empty
  // channel).
  bool blocked;

};

#endif /* DECODER_H_ */
