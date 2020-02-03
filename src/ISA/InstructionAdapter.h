/*
 * InstructionAdapter.h
 *
 * Allow an InstructionBase to implement the InstructionInterface, following the
 * mix-in paradigm.
 *
 * It would be possible for InstructionBase to implement InstructionInterface
 * directly, but then there would be a large number of virtual function calls,
 * and the compiler would not be able to optimise across functions effectively.
 *
 *  Created on: 5 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_INSTRUCTIONADAPTER_H_
#define SRC_ISA_INSTRUCTIONADAPTER_H_

#include "InstructionInterface.h"

template<class T>
class InstructionAdapter : public InstructionInterface, public T {
public:
  InstructionAdapter(Instruction encoded) : T(encoded) {}
  virtual ~InstructionAdapter() {}

  // Prepare the instruction for execution on a given core. Also provide some
  // extra information to help with debugging.
  virtual void assignToCore(CoreInterface& core, MemoryAddr address,
                            InstructionSource source) {
    T::assignToCore(core, address, source);
  }

  // Can this instruction be squashed based on the value of the predicate?
  virtual bool isPredicated() const {return T::isPredicated();}

  // Is this instruction the final one in its instruction packet?
  virtual bool isEndOfPacket() const {return T::isEndOfPacket();}

  // Does this instruction read from the register file?
  virtual bool readsRegister(RegisterPort port) const {
    return T::readsRegister(port);
  }

  // Does this instruction write to the register file?
  virtual bool writesRegister() const {return T::writesRegister();}

  // Does this instruction read the predicate bit? (In order to compute a
  // result, not to see if the instruction should execute or not.)
  virtual bool readsPredicate() const {return T::readsPredicate();}

  // Does this instruction write the predicate bit?
  virtual bool writesPredicate() const {return T::writesPredicate();}

  // Does this instruction read from the channel map table?
  virtual bool readsCMT() const {return T::readsCMT();}

  // Does this instruction write to the channel map table?
  virtual bool writesCMT() const {return T::writesCMT();}

  // Does this instruction read the control registers?
  virtual bool readsCReg() const {return T::readsCReg();}

  // Does this instruction write to the control registers?
  virtual bool writesCReg() const {return T::writesCReg();}

  // Does this instruction read from the scratchpad?
  virtual bool readsScratchpad() const {return T::readsScratchpad();}

  // Does this instruction write to the scratchpad?
  virtual bool writesScratchpad() const {return T::writesScratchpad();}

  // Does this instruction send data onto the network?
  // (Fetches don't count - the network access is a side effect of checking the
  // cache tags.)
  virtual bool sendsOnNetwork() const {return T::sendsOnNetwork();}


  // Return the index of the register read from the given port.
  virtual RegisterIndex getSourceRegister(RegisterPort port) const {
    return T::getSourceRegister(port);
  }

  // Return the index of the register written.
  virtual RegisterIndex getDestinationRegister() const {
    return T::getDestinationRegister();
  }

  // Return the computed result of the instruction.
  virtual int32_t getResult() const {return T::getResult();}


  // Returns whether the instruction is blocked on some operation. An
  // instruction may only be passed down the pipeline if it is not busy.
  virtual bool busy() const {return T::busy();}

  // Event triggered whenever a phase of computation completes. At most one
  // phase may be in progress at a time.
  virtual const sc_core::sc_event& finishedPhaseEvent() const {
    return T::finishedPhaseEvent();
  }

  // Check whether a particular phase of execution has completed.
  virtual bool completedPhase(ExecutionPhase phase) const {
    return T::completedPhase(phase);
  }

  // Read registers, including register-mapped input FIFOs.
  virtual void readRegisters() {T::readRegisters();}
  virtual void readRegistersCallback(RegisterPort port, int32_t value) {
    T::readRegistersCallback(port, value);
  }

  // Write to the register file.
  virtual void writeRegisters() {T::writeRegisters();}
  virtual void writeRegistersCallback() {T::writeRegistersCallback();}

  // Computation which happens before the execute stage (e.g. fetch, selch).
  virtual void earlyCompute() {T::earlyCompute();}
  virtual void earlyComputeCallback(int32_t value=0) {
    T::earlyComputeCallback(value);
  }

  // Main computation.
  virtual void compute() {T::compute();}
  virtual void computeCallback(int32_t value=0) {T::computeCallback(value);}

  // Read channel map table.
  virtual void readCMT() {T::readCMT();}
  virtual void readCMTCallback(EncodedCMTEntry value) {
    T::readCMTCallback(value);
  }

  // Write to the channel map table.
  virtual void writeCMT() {T::writeCMT();}
  virtual void writeCMTCallback() {T::writeCMTCallback();}

  // Read the core-local scratchpad.
  virtual void readScratchpad() {T::readScratchpad();}
  virtual void readScratchpadCallback(int32_t value) {
    T::readScratchpadCallback(value);
  }

  // Write to the core-local scratchpad.
  virtual void writeScratchpad() {T::writeScratchpad();}
  virtual void writeScratchpadCallback() {T::writeScratchpadCallback();}

  // Read the control registers.
  virtual void readCregs() {T::readCregs();}
  virtual void readCregsCallback(int32_t value) {T::readCregsCallback(value);}

  // Write to the control registers.
  virtual void writeCregs() {T::writeCregs();}
  virtual void writeCregsCallback() {T::writeCregsCallback();}

  // Read the control registers.
  virtual void readPredicate() {T::readPredicate();}
  virtual void readPredicateCallback(bool value) {
    T::readPredicateCallback(value);
  }

  // Write to the predicate register.
  virtual void writePredicate() {T::writePredicate();}
  virtual void writePredicateCallback() {T::writePredicateCallback();}

  // Send data onto the network.
  virtual void sendNetworkData() {T::sendNetworkData();}
  virtual void sendNetworkDataCallback() {T::sendNetworkDataCallback();}

  // Notify that a credit arrived.
  virtual void creditArrivedCallback() {T::creditArrivedCallback();}
};



#endif /* SRC_ISA_INSTRUCTIONADAPTER_H_ */
