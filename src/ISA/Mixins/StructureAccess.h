/*
 * StructureAccess.h
 *
 * Mix-ins which allow access to the various storage structures in the Core.
 *  * Register file (writing only - reading happens in OperandSource.h)
 *  * Predicate register
 *  * Control registers
 *  * Channel map table
 *  * Scratchpad
 *
 *  Created on: 5 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_MIXINS_STRUCTUREACCESS_H_
#define SRC_ISA_MIXINS_STRUCTUREACCESS_H_


// An instruction which writes to the register file.
// This mix-in must wrap an OperandSource which specifies a destination.
template<class T>
class WriteRegister : public T {
protected:
  WriteRegister(Instruction encoded) : T(encoded) {}

  void writeRegisters() {
    this->core->writeRegister(this->destinationRegister, this->result);
  }

  void writeRegistersCallback() {
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};


// An instruction which reads the core's predicate bit.
template<class T>
class ReadPredicate : public T {
protected:
  ReadPredicate(Instruction encoded) : T(encoded) {predicateBit = false;}

  void readPredicate() {
    this->core->readPredicate();
  }

  void readPredicateCallback(bool value) {
    predicateBit = value;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }

  bool predicateBit;
};


// An instruction which writes the core's predicate bit.
// This mix-in must wrap any which define how computation is performed.
template<class T>
class WritePredicate : public T {
protected:
  WritePredicate(Instruction encoded) : T(encoded) {newPredicate = false;}

  void compute() {
    T::compute();
    this->computePredicate();
  }

  void writePredicate() {
    this->core->writePredicate(newPredicate);
  }

  void writePredicateCallback() {
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }

  bool newPredicate;
};


template<class T>
class ReadCMT : public T {
protected:
  ReadCMT(Instruction encoded) : T(encoded) {channelMapping = 0;}

  void readCMT() {
    if (this->outChannel == NO_CHANNEL)
      this->finished.notify(sc_core::SC_ZERO_TIME);
    else
      this->core->readCMT(this->outChannel);
  }

  void readCMTCallback(EncodedCMTEntry value) {
    channelMapping = value;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }

  EncodedCMTEntry channelMapping;
};

template<class T>
class WriteCMT : public T {
protected:
  WriteCMT(Instruction encoded) : T(encoded) {}

  void writeCMT() {
    RegisterIndex entry = this->operand2;
    EncodedCMTEntry value = this->operand1;
    this->core->writeCMT(entry, value);
  }

  void writeCMTCallback() {
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};


template<class T>
class ReadCregs : public T {
protected:
  ReadCregs(Instruction encoded) : T(encoded) {}

  void readCregs() {
    this->core->readCreg(this->operand1);
  }

  void readCregsCallback(int32_t value) {
    this->result = value;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};

template<class T>
class WriteCregs : public T {
protected:
  WriteCregs(Instruction encoded) : T(encoded) {}

  void writeCregs() {
    RegisterIndex entry = this->operand2;
    int32_t value = this->operand1;
    this->core->writeCreg(entry, value);
  }

  void writeCregsCallback() {
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};


template<class T>
class ReadScratchpad : public T {
protected:
  ReadScratchpad(Instruction encoded) : T(encoded) {}

  void readScratchpad() {
    this->core->readScratchpad(this->operand1);
  }

  void readScratchpadCallback(int32_t value) {
    this->result = value;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};

template<class T>
class WriteScratchpad : public T {
protected:
  WriteScratchpad(Instruction encoded) : T(encoded) {}

  void writeScratchpad() {
    RegisterIndex entry = this->operand2;
    int32_t value = this->operand1;
    this->core->writeScratchpad(entry, value);
  }

  void writeScratchpadCallback() {
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};



#endif /* SRC_ISA_MIXINS_STRUCTUREACCESS_H_ */
