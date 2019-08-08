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
  virtual ~InstructionInterface() = 0;

  // Prepare the instruction for execution on a given core. Also provide some
  // extra information to help with debugging.
  virtual void assignToCore(CoreInterface& core, MemoryAddr address,
                            InstructionSource source) = 0;

  // Event triggered whenever a phase of computation completes. At most one
  // phase may be in progress at a time.
  virtual const sc_core::sc_event& finishedPhaseEvent() const = 0;

  // Read registers, including register-mapped input FIFOs.
  virtual void readRegisters() = 0;
  virtual void readRegistersCallback(RegisterPort port, int32_t value) = 0;

  // Write to the register file.
  virtual void writeRegisters() = 0;
  virtual void writeRegistersCallback() = 0;

  // Computation which happens before the execute stage (e.g. fetch, selch).
  virtual void earlyCompute() = 0;
  virtual void earlyComputeCallback(int32_t value=0) = 0;

  // Main computation.
  virtual void compute() = 0;
  virtual void computeCallback(int32_t value=0) = 0;

  // Read channel map table.
  virtual void readCMT() = 0;
  virtual void readCMTCallback(EncodedCMTEntry value) = 0;

  // Write to the channel map table.
  virtual void writeCMT() = 0;
  virtual void writeCMTCallback() = 0;

  // Read the core-local scratchpad.
  virtual void readScratchpad() = 0;
  virtual void readScratchpadCallback(int32_t value) = 0;

  // Write to the core-local scratchpad.
  virtual void writeScratchpad() = 0;
  virtual void writeScratchpadCallback() = 0;

  // Read the control registers.
  virtual void readCregs() = 0;
  virtual void readCregsCallback(int32_t value) = 0;

  // Write to the control registers.
  virtual void writeCregs() = 0;
  virtual void writeCregsCallback() = 0;

  // Read the control registers.
  virtual void readPredicate() = 0;
  virtual void readPredicateCallback(bool value) = 0;

  // Write to the predicate register.
  virtual void writePredicate() = 0;
  virtual void writePredicateCallback() = 0;

  // Send data onto the network.
  virtual void sendNetworkData() = 0;
  virtual void sendNetworkDataCallback() = 0;
};



#endif /* SRC_ISA_INSTRUCTIONINTERFACE_H_ */
