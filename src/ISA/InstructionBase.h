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
#include <bitset>

class InstructionBase {
protected:
  InstructionBase(Instruction encoded);

public:
  // Prepare the instruction for execution on a given core. Also provide some
  // extra information to help with debugging.
  void assignToCore(CoreInterface& core, MemoryAddr address,
                    InstructionSource source);

  // Can this instruction be squashed based on the value of the predicate?
  bool isPredicated() const;

  // Is this instruction the final one in its instruction packet?
  bool isEndOfPacket() const;

  // Does this instruction read from the register file?
  bool readsRegister(PortIndex port) const;

  // Does this instruction write to the register file?
  bool writesRegister() const;

  // Does this instruction read the predicate bit? (In order to compute a
  // result, not to see if the instruction should execute or not.)
  bool readsPredicate() const;

  // Does this instruction write the predicate bit?
  bool writesPredicate() const;

  // Does this instruction read from the channel map table?
  bool readsCMT() const;

  // Does this instruction write to the channel map table?
  bool writesCMT() const;

  // Does this instruction read the control registers?
  bool readsCReg() const;

  // Does this instruction write to the control registers?
  bool writesCReg() const;

  // Does this instruction read from the scratchpad?
  bool readsScratchpad() const;

  // Does this instruction write to the scratchpad?
  bool writesScratchpad() const;

  // Does this instruction send data onto the network?
  // (Fetches don't count - the network access is a side effect of checking the
  // cache tags.)
  bool sendsOnNetwork() const;


  // Return the index of the register read from the given port.
  RegisterIndex getSourceRegister(PortIndex port) const;

  // Return the index of the register written.
  RegisterIndex getDestinationRegister() const;

  // Return the computed result of the instruction.
  int32_t getResult() const;


  // Event triggered whenever a phase of computation completes. Multiple
  // phases may be in progress at a time.
  const sc_core::sc_event& finishedPhaseEvent() const;

  // Check whether a particular phase of execution has completed.
  bool completedPhase(InstructionInterface::ExecutionPhase phase) const;

  // Returns whether the instruction is blocked on some operation. An
  // instruction may only be passed down the pipeline if it is not busy.
  bool busy() const;

  // Read registers, including register-mapped input FIFOs.
  void readRegisters();
  void readRegistersCallback(PortIndex port, int32_t value);

  // Write to the register file.
  void writeRegisters();
  void writeRegistersCallback(PortIndex port);

  // Computation which happens before the execute stage (e.g. fetch, selch).
  void earlyCompute();
  void earlyComputeCallback(int32_t value=0);

  // Main computation.
  void compute();
  void computeCallback(int32_t value=0);

  // Read channel map table.
  void readCMT();
  void readCMTCallback(PortIndex port, EncodedCMTEntry value);

  // Write to the channel map table.
  void writeCMT();
  void writeCMTCallback(PortIndex port);

  // Read the core-local scratchpad.
  void readScratchpad();
  void readScratchpadCallback(PortIndex port, int32_t value);

  // Write to the core-local scratchpad.
  void writeScratchpad();
  void writeScratchpadCallback(PortIndex port);

  // Read the control registers.
  void readCregs();
  void readCregsCallback(PortIndex port, int32_t value);

  // Write to the control registers.
  void writeCregs();
  void writeCregsCallback(PortIndex port);

  // Read the control registers.
  void readPredicate();
  void readPredicateCallback(PortIndex port, bool value);

  // Write to the predicate register.
  void writePredicate();
  void writePredicateCallback(PortIndex port);

  // Send data onto the network.
  void sendNetworkData();
  void sendNetworkDataCallback();

  // Notify that a credit arrived.
  void creditArrivedCallback();

protected:

  // If this instruction takes a signed immediate, extend the lowest `bits`
  // bits of `value` into a full 32 bit signed integer. Otherwise return value.
  int32_t signExtend(int32_t value, size_t bits) const;

  void startingPhase(InstructionInterface::ExecutionPhase phase);
  void finishedPhase(InstructionInterface::ExecutionPhase phase);
  bool phaseInProgress(InstructionInterface::ExecutionPhase phase) const;
  std::bitset<InstructionInterface::NUM_PHASES> phasesInProgress;
  std::bitset<InstructionInterface::NUM_PHASES> phasesComplete;

  // The original instruction.
  const Instruction encoded;

  // The instruction's opcode. Note that some instructions extend this with
  // a few function bits.
  const opcode_t opcode;

  // The conditions under which this instruction will execute.
  const Predicate predicate;

  // Event triggered whenever a phase of computation completes.
  sc_core::sc_event finished;

  CoreInterface* core;      // The core this instruction is executing on
  MemoryAddr location;      // The memory address the instruction came from
  InstructionSource source; // Whether the instruction came from FIFO or cache
};


#endif /* SRC_ISA_INSTRUCTIONBASE_H_ */
