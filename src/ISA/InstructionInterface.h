/*
 * InstructionInterface.h
 *
 * Interface for all instructions.
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
 *  Created on: 5 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_INSTRUCTIONINTERFACE_H_
#define SRC_ISA_INSTRUCTIONINTERFACE_H_

#include "systemc"
#include "../Tile/ChannelMapEntry.h"
#include "../Tile/Core/Fetch/InstructionStore.h"
#include "CoreInterface.h"

class InstructionInterface {

public:
  enum ExecutionPhase {
    INST_FETCH,
    INST_DECODE,
    INST_REG_READ,
    INST_REG_WRITE,
    INST_CMT_READ,
    INST_CMT_WRITE,
    INST_PRED_READ,
    INST_PRED_WRITE,
    INST_SPAD_READ,
    INST_SPAD_WRITE,
    INST_CREG_READ,
    INST_CREG_WRITE,
    INST_COMPUTE,
    INST_NETWORK_SEND,

    NUM_PHASES
  };

  virtual ~InstructionInterface() = 0;

  // Prepare the instruction for execution on a given core. Also provide some
  // extra information to help with debugging.
  virtual void assignToCore(CoreInterface& core, MemoryAddr address,
                            InstructionSource source) = 0;


//============================================================================//
// Queries - inspect the state of an instruction.
//
// For the most part, the core should not need to use these as the instruction
// will handle things itself. e.g. `readRegisters()` should be executed
// regardless of whether `readsRegister()` is true or not.
//
// These methods are useful for allowing instructions to interact.
// e.g. If one instruction `writesPredicate()` and the next instruction
// `readsPredicate()`, we know we need to stall or forward the result.
//============================================================================//

  // Can this instruction be squashed based on the value of the predicate?
  virtual bool isPredicated() const = 0;

  // Is this instruction the final one in its instruction packet?
  virtual bool isEndOfPacket() const = 0;

  // Does this instruction read from the register file?
  virtual bool readsRegister(PortIndex port) const = 0;

  // Does this instruction write to the register file?
  virtual bool writesRegister() const = 0;

  // Does this instruction read the predicate bit? (In order to compute a
  // result, not to see if the instruction should execute or not.)
  virtual bool readsPredicate() const = 0;

  // Does this instruction write the predicate bit?
  virtual bool writesPredicate() const = 0;

  // Does this instruction read from the channel map table?
  virtual bool readsCMT() const = 0;

  // Does this instruction write to the channel map table?
  virtual bool writesCMT() const = 0;

  // Does this instruction read the control registers?
  virtual bool readsCReg() const = 0;

  // Does this instruction write to the control registers?
  virtual bool writesCReg() const = 0;

  // Does this instruction read from the scratchpad?
  virtual bool readsScratchpad() const = 0;

  // Does this instruction write to the scratchpad?
  virtual bool writesScratchpad() const = 0;

  // Does this instruction send data onto the network?
  // (Fetches don't count - the network access is a side effect of checking the
  // cache tags.)
  virtual bool sendsOnNetwork() const = 0;


  // Return the index of the register read from the given port.
  virtual RegisterIndex getSourceRegister(PortIndex port) const = 0;

  // Return the index of the register written.
  virtual RegisterIndex getDestinationRegister() const = 0;

  // Return the computed result of the instruction.
  virtual int32_t getResult() const = 0;


  // Event triggered whenever a phase of computation completes.
  virtual const sc_core::sc_event& finishedPhaseEvent() const = 0;

  // Check whether a particular phase of execution has completed.
  virtual bool completedPhase(ExecutionPhase phase) const = 0;

  // Returns whether the instruction is blocked on some operation. An
  // instruction may only be passed down the pipeline if it is not busy.
  virtual bool busy() const = 0;


//============================================================================//
// Execution - trigger phases of computation and return results to the
// instruction.
//============================================================================//

  // Read registers, including register-mapped input FIFOs.
  virtual void readRegisters() = 0;
  virtual void readRegistersCallback(PortIndex port, int32_t value) = 0;

  // Write to the register file.
  virtual void writeRegisters() = 0;
  virtual void writeRegistersCallback(PortIndex port) = 0;

  // Computation which happens before the execute stage (e.g. fetch, selch).
  virtual void earlyCompute() = 0;
  virtual void earlyComputeCallback(int32_t value=0) = 0;

  // Main computation.
  virtual void compute() = 0;
  virtual void computeCallback(int32_t value=0) = 0;

  // Read channel map table.
  virtual void readCMT() = 0;
  virtual void readCMTCallback(PortIndex port, EncodedCMTEntry value) = 0;

  // Write to the channel map table.
  virtual void writeCMT() = 0;
  virtual void writeCMTCallback(PortIndex port) = 0;

  // Read the core-local scratchpad.
  virtual void readScratchpad() = 0;
  virtual void readScratchpadCallback(PortIndex port, int32_t value) = 0;

  // Write to the core-local scratchpad.
  virtual void writeScratchpad() = 0;
  virtual void writeScratchpadCallback(PortIndex port) = 0;

  // Read the control registers.
  virtual void readCregs() = 0;
  virtual void readCregsCallback(PortIndex port, int32_t value) = 0;

  // Write to the control registers.
  virtual void writeCregs() = 0;
  virtual void writeCregsCallback(PortIndex port) = 0;

  // Read the control registers.
  virtual void readPredicate() = 0;
  virtual void readPredicateCallback(PortIndex port, bool value) = 0;

  // Write to the predicate register.
  virtual void writePredicate() = 0;
  virtual void writePredicateCallback(PortIndex port) = 0;

  // Send data onto the network.
  virtual void sendNetworkData() = 0;
  virtual void sendNetworkDataCallback() = 0;

  // Notify that a credit arrived.
  virtual void creditArrivedCallback() = 0;
};


#endif /* SRC_ISA_INSTRUCTIONINTERFACE_H_ */
