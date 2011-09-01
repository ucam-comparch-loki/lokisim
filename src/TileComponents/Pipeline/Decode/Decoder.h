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
#include "../../../Utility/InstructionMap.h"

class DecodedInst;
class DecodeStage;
class Instruction;

class Decoder: public Component {

//==============================//
// Methods
//==============================//

public:

  // Extract information from the input instruction and determine what the
  // operands are to be. Stores result in output. Returns whether there
  // is useful output this cycle -- the first cycle of multi-cycle operations
  // will return false.
  bool decodeInstruction(const DecodedInst& input, DecodedInst& output);

  // Returns whether the decoder is ready to accept a new instruction.
  bool ready() const;
  const sc_event& stalledEvent() const;

private:

  // Wait for all operands to arrive. One or more operands may come from the
  // network, or need to be forwarded from another pipeline stage, and so may
  // not be ready when this instruction first reaches the decoder.
  void waitForOperands(const DecodedInst& dec);

  // Determine whether to read the first operand from the receive channel-end
  // table, the ALU or the register file.
  void setOperand1(DecodedInst& dec);

  // Determine whether to read the second operand from the receive channel-end
  // table, the ALU, the register file, or the sign extender.
  void setOperand2(DecodedInst& dec);

  // Read a register value.
  int32_t readRegs(RegisterIndex index, bool indirect = false);

  // Wait until there is data in a particular input buffer.
  void    waitUntilArrival(ChannelIndex channel);

  // Prepare this instruction to be sent to a remote core.
  void    remoteExecution(DecodedInst& instruction) const;

  // Ensure that the instruction packet from the given address is in the
  // instruction packet cache. Nothing will be done if it is already there.
  // There are many different ways of fetching instructions, so provide the
  // operation too.
  void    fetch(MemoryAddr addr, operation_t operation) const;

  // Update the channel to which we send fetch requests.
  void    setFetchChannel(uint32_t encodedChannel) const;

  // If an instruction is waiting to be decoded, discard it. Returns whether
  // anything was discarded.
  bool    discardNextInst() const;

  // Write operations take two cycles since there are two flits to send. This
  // method sends the second part.
  bool continueOp(const DecodedInst& input, DecodedInst& output);

  // Determine whether the current instruction should be executed, based on its
  // predicate bits, and the contents of the predicate register.
  bool shouldExecute(const DecodedInst& inst);

  void stall(bool stall, int reason = 0);

  DecodeStage* parent() const;

//==============================//
// Constructors and destructors
//==============================//

public:

  Decoder(const sc_module_name& name, const ComponentID& id);

//==============================//
// Local state
//==============================//

private:

  // The remote channel we are sending instructions to.
  ChannelIndex sendChannel;

  // The location in memory to fetch instructions from.
  MemoryAddr fetchAddress;

  // Tells whether or not we are in remote execution mode.
  bool remoteExecute;

  // Tells whether we have started a two-cycle store operation.
  bool multiCycleOp;

  // Tells whether we are blocked (probably trying to read from an empty
  // channel).
  bool blocked;
  sc_core::sc_event blockedEvent;

};

#endif /* DECODER_H_ */
