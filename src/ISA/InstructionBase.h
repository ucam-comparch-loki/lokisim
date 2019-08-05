/*
 * InstructionBase.h
 *
 * Base class for all instructions. Intended for the mix-in programming pattern.
 *
 * This class provides dummy implementations of all methods, which do nothing,
 * or throw errors. This class should be extended to implement functionality.
 *
 * All methods for InstructionInterface are provided, but to allow better
 * optimisation, they are all non-virtual. An InstructionAdapter should be used
 * to make an InstructionBase implement the InstructionInterface.
 *
 *  Created on: 2 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_INSTRUCTIONBASE_H_
#define SRC_ISA_INSTRUCTIONBASE_H_

#include "InstructionInterface.h"
#include "../Datatype/Instruction.h"

class InstructionBase {
protected:
  InstructionBase(Instruction encoded);

public:
  // Prepare the instruction for execution on a given core. Also provide some
  // extra information to help with debugging.
  void assignToCore(CoreInterface& core, MemoryAddr address,
                    InstructionSource source);

  // Event triggered whenever a phase of computation completes. At most one
  // phase may be in progress at a time.
  const sc_core::sc_event& finishedPhaseEvent() const;

  // Read registers, including register-mapped input FIFOs.
  void readRegisters();
  void readRegistersCallback(RegisterPort port, int32_t value);

  // Write to the register file.
  void writeRegisters();
  void writeRegistersCallback();

  // Computation which happens before the execute stage (e.g. fetch, selch).
  void earlyCompute();
  void earlyComputeCallback();

  // Main computation.
  void compute();
  void computeCallback();

  // Read channel map table.
  void readCMT();
  void readCMTCallback(EncodedCMTEntry value);

  // Write to the channel map table.
  void writeCMT();
  void writeCMTCallback();

  // Read the core-local scratchpad.
  void readScratchpad();
  void readScratchpadCallback(int32_t value);

  // Write to the core-local scratchpad.
  void writeScratchpad();
  void writeScratchpadCallback();

  // Read the control registers.
  void readCregs();
  void readCregsCallback(int32_t value);

  // Write to the control registers.
  void writeCregs();
  void writeCregsCallback();

  // Read the control registers.
  void readPredicate();
  void readPredicateCallback(bool value);

  // Write to the predicate register.
  void writePredicate();
  void writePredicateCallback();

  // Send data onto the network.
  void sendNetworkData();
  void sendNetworkDataCallback();

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
