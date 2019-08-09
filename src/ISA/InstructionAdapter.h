/*
 * InstructionAdapter.h
 *
 * Allow an InstructionBase to implement the InstructionInterface.
 *
 * It would be possible for InstructionBase to implement InstructionInterface
 * directly, but then there would be a large number of virtual function calls.
 *
 *  Created on: 5 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_INSTRUCTIONADAPTER_H_
#define SRC_ISA_INSTRUCTIONADAPTER_H_

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

  // Event triggered whenever a phase of computation completes. At most one
  // phase may be in progress at a time.
  virtual const sc_core::sc_event& finishedPhaseEvent() const {
    return T::finishedPhaseEvent();
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
};



#endif /* SRC_ISA_INSTRUCTIONADAPTER_H_ */
