/*
 * InstructionBase.h
 *
 * Base class for all instructions.
 *
 * In order to create a class for an instruction, we must have already fetched
 * and decoded it, so only execution steps after that point are included.
 *
 * Methods come in groups of two:
 *  1. doSomething()
 *      Request that the instruction completes a particular phase of execution.
 *  2. doSomethingCallback()
 *      Tell instruction that one step of this phase is complete.
 *      Depending on the phase, data may also be passed to the instruction.
 *
 *  Created on: 2 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_INSTRUCTIONBASE_H_
#define SRC_ISA_INSTRUCTIONBASE_H_

#include "systemc"
#include "../Datatype/Instruction.h"
#include "../Tile/ChannelMapEntry.h"
#include "../Tile/Core/Fetch/InstructionStore.h"
#include "CoreInterface.h"

class InstructionBase {
protected:
  InstructionBase(Instruction encoded);
  virtual ~InstructionBase();

public:
  // Prepare the instruction for execution on a given core. Also provide some
  // extra information to help with debugging.
  void assignToCore(CoreInterface& core, MemoryAddr address,
                    InstructionSource source);

  // Event triggered whenever a phase of computation completes. At most one
  // phase may be in progress at a time.
  const sc_core::sc_event& finishedPhaseEvent() const;

  // Read registers, including register-mapped input FIFOs.
  virtual void readRegisters();
  virtual void readRegistersCallback(RegisterPort port, int32_t value);

  // Write to the register file.
  virtual void writeRegisters();
  virtual void writeRegistersCallback();

  // Computation which happens before the execute stage (e.g. fetch, selch).
  virtual void earlyCompute();
  virtual void earlyComputeCallback();

  // Main computation.
  virtual void compute();
  virtual void computeCallback();

  // Read channel map table.
  virtual void readCMT();
  virtual void readCMTCallback(EncodedCMTEntry value);

  // Write to the channel map table.
  virtual void writeCMT();
  virtual void writeCMTCallback();

  // Read the core-local scratchpad.
  virtual void readScratchpad();
  virtual void readScratchpadCallback(int32_t value);

  // Write to the core-local scratchpad.
  virtual void writeScratchpad();
  virtual void writeScratchpadCallback();

  // Read the control registers.
  virtual void readCregs();
  virtual void readCregsCallback(int32_t value);

  // Write to the control registers.
  virtual void writeCregs();
  virtual void writeCregsCallback();

  // Read the control registers.
  virtual void readPredicate();
  virtual void readPredicateCallback(bool value);

  // Write to the predicate register.
  virtual void writePredicate();
  virtual void writePredicateCallback();

  // Send data onto the network.
  virtual void sendNetworkData();
  virtual void sendNetworkDataCallback();

protected:

  // If this instruction takes a signed immediate, extend the lowest `bits`
  // bits of `value` into a full 32 bit signed integer. Otherwise return value.
  int32_t signExtend(int32_t value, size_t bits) const;

  // The original instruction.
  const Instruction encoded;

  // The instruction's opcode. Note that some instructions extend this with
  // a few function bits.
  const opcode_t opcode;

  // The conditions under which this instruction will execute.
  const Predicate predicate;

  // Event triggered whenever a phase of computation completes. At most one
  // phase may be in progress at a time.
  sc_core::sc_event finished;

  CoreInterface* core;      // The core this instruction is executing on
  MemoryAddr location;      // The memory address the instruction came from
  InstructionSource source; // Whether the instruction came from FIFO or cache
};


#endif /* SRC_ISA_INSTRUCTIONBASE_H_ */
