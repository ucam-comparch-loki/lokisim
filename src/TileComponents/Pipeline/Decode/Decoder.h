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
#include "../../../Utility/Blocking.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Utility/Instrumentation/Stalls.h"

class DecodeStage;

class Decoder: public Component, public Blocking {

//==============================//
// Methods
//==============================//

public:

  void instructionFinished();
  bool needPredicateNow(const DecodedInst& inst) const;
  bool allOperandsReady() const;
  bool hasOutput() const;
  const DecodedInst& getOutput() const;
  bool checkChannelInput(ChannelIndex channel, const DecodedInst& inst);
  void waitForOperands2(const DecodedInst& inst);

  // Extract information from the input instruction and determine what the
  // operands are to be. Stores result in output. Returns whether there
  // is useful output this cycle -- the first cycle of multi-cycle operations
  // will return false.
  bool decodeInstruction(const DecodedInst& input, DecodedInst& output);

  // Abort the instruction which is currently decoding (if any), even if it is
  // stalled waiting for data to arrive.
  void cancelInstruction();

  // Returns whether the decoder is ready to accept a new instruction.
  bool ready() const;
  const sc_event& stalledEvent() const;

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

protected:

  virtual void reportStalls(ostream& os);

private:

  // Read a register value.
  int32_t readRegs(PortIndex port, RegisterIndex index, bool indirect = false);

  // Wait until there is data in a particular input buffer.
  void    waitUntilArrival(ChannelIndex channel, const DecodedInst& inst);

  // If an instruction is waiting to be decoded, discard it. Returns whether
  // anything was discarded.
  bool    discardNextInst() const;

  // Write operations take two cycles since there are two flits to send. This
  // method sends the second part.
  bool continueOp(const DecodedInst& input, DecodedInst& output);

  // Returns whether we expect data arriving at a particular buffer to be coming
  // from memory. ChannelIndex 0 is mapped to r2.
  bool connectionFromMemory(ChannelIndex buffer) const;

  // Determine whether the current instruction should be executed, based on its
  // predicate bits, and the contents of the predicate register.
  bool shouldExecute(const DecodedInst& inst);

  // Determine whether the given opcode may result in a fetch request being sent.
  bool isFetch(opcode_t opcode) const;

  // Perform a fetch.
  void fetch(DecodedInst& inst);

  void stall(bool stall, Instrumentation::Stalls::StallReason reason, const DecodedInst& cause);

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

  // None of the following are needed in hardware - they are just required here
  // because of the way decoding has been split up into multiple blocks.
  // Could instead use a collection of possible states.
  DecodedInst current, output, previous;
  bool continueToExecute, execute, haveAllOperands;
  int outputsRemaining;
  bool needDestination, needOperand1, needOperand2, needChannel;

  // Tells whether we have started a two-cycle store operation.
  bool multiCycleOp;

  // Tells whether we are blocked (probably trying to read from an empty
  // channel).
  bool blocked;
  sc_event blockedEvent;

public:
  bool instructionCancelled;
private:
  sc_event cancelEvent;

  // If the previous instruction was predicated, we may not know whether
  // forwarding will be possible, so must be pessimistic and read registers too.
  bool previousInstPredicated;
  bool previousInstSetPredicate;

};

#endif /* DECODER_H_ */
