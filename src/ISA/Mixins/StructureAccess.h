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

#include "../../Datatype/Instruction.h"
#include "../../Tile/ChannelMapEntry.h"


// An instruction which reads the core's predicate bit.
template<class T>
class ReadPredicate : public T {
public:
  ReadPredicate(Instruction encoded) : T(encoded) {predicateBit = false;}

  void readPredicate() {
    this->startingPhase(this->INST_PRED_READ);
    this->core->readPredicate();
  }

  void readPredicateCallback(bool value) {
    predicateBit = value;
    this->finishedPhase(this->INST_PRED_READ);
  }

protected:
  bool predicateBit;
};


// An instruction which writes the core's predicate bit.
// This mix-in must wrap any which define how computation is performed.
template<class T>
class WritePredicate : public T {
public:
  WritePredicate(Instruction encoded) : T(encoded) {newPredicate = false;}

  void compute() {
    // The result of `compute` is used in multiple places, so can't be returned.
    // The result of `computePredicate` is only used here, so we can encapsulate
    // it better.

    T::compute();
    newPredicate = this->computePredicate();
  }

  void writePredicate() {
    this->startingPhase(this->INST_PRED_WRITE);
    this->core->writePredicate(newPredicate);
  }

  void writePredicateCallback() {
    this->finishedPhase(this->INST_PRED_WRITE);
  }

protected:
  bool newPredicate;
};


template<class T>
class ReadCMT : public T {
public:
  ReadCMT(Instruction encoded) : T(encoded) {channelMapping = 0;}

  void readCMT() {
    this->startingPhase(this->INST_CMT_READ);

    if (this->outChannel == NO_CHANNEL)
      this->finishedPhase(this->INST_CMT_READ);
    else
      this->core->readCMT(this->outChannel);
  }

  void readCMTCallback(EncodedCMTEntry value) {
    channelMapping = value;
    this->finishedPhase(this->INST_CMT_READ);
  }

protected:
  EncodedCMTEntry channelMapping;
};

template<class T>
class WriteCMT : public Has2Operands<T> {
public:
  WriteCMT(Instruction encoded) : Has2Operands<T>(encoded) {}

  void writeCMT() {
    this->startingPhase(this->INST_CMT_WRITE);
    RegisterIndex entry = this->operand2;
    EncodedCMTEntry value = this->operand1;
    this->core->writeCMT(entry, value);
  }

  void writeCMTCallback() {
    this->finishedPhase(this->INST_CMT_WRITE);
  }
};


template<class T>
class ReadCregs : public Has1Operand<HasResult<T>> {
public:
  ReadCregs(Instruction encoded) : Has1Operand<HasResult<T>>(encoded) {}

  void readCregs() {
    this->startingPhase(this->INST_CREG_READ);
    this->core->readCreg(this->operand1);
  }

  void readCregsCallback(int32_t value) {
    this->result = value;
    this->finishedPhase(this->INST_CREG_READ);
  }
};

template<class T>
class WriteCregs : public Has2Operands<T> {
public:
  WriteCregs(Instruction encoded) : Has2Operands<T>(encoded) {}

  void writeCregs() {
    this->startingPhase(this->INST_CREG_WRITE);
    RegisterIndex entry = this->operand2;
    int32_t value = this->operand1;
    this->core->writeCreg(entry, value);
  }

  void writeCregsCallback() {
    this->finishedPhase(this->INST_CREG_WRITE);
  }
};


template<class T>
class ReadScratchpad : public Has1Operand<HasResult<T>> {
public:
  ReadScratchpad(Instruction encoded) : Has1Operand<HasResult<T>>(encoded) {}

  void readScratchpad() {
    this->startingPhase(this->INST_SPAD_READ);
    this->core->readScratchpad(this->operand1);
  }

  void readScratchpadCallback(int32_t value) {
    this->result = value;
    this->finishedPhase(this->INST_SPAD_READ);
  }
};

template<class T>
class WriteScratchpad : public Has2Operands<T> {
public:
  WriteScratchpad(Instruction encoded) : Has2Operands<T>(encoded) {}

  void writeScratchpad() {
    this->startingPhase(this->INST_SPAD_WRITE);
    RegisterIndex entry = this->operand2;
    int32_t value = this->operand1;
    this->core->writeScratchpad(entry, value);
  }

  void writeScratchpadCallback() {
    this->finishedPhase(this->INST_SPAD_WRITE);
  }
};



#endif /* SRC_ISA_MIXINS_STRUCTUREACCESS_H_ */
