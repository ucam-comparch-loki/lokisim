/*
 * Network.h
 *
 * Operations which directly interact with the on-chip network.
 *
 * Many other instructions can receive operands from the network or send results
 * onto the network, but do not generally rely on the existence of the network.
 *
 *  Created on: 7 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_OPERATIONS_NETWORK_H_
#define SRC_ISA_OPERATIONS_NETWORK_H_

#include "../../Datatype/Instruction.h"

namespace ISA {

// Wait on channel end. Block the pipeline until the given output channel
// has at least a required number of credits.
template<class T>
class WaitOnChannelEnd : public Has1Operand<T> {
public:
  WaitOnChannelEnd(Instruction encoded) : Has1Operand<T>(encoded) {}

  void compute() {
    this->startingPhase(this->INST_COMPUTE);
    checkIfFinished();
  }
  void computeCallback(int32_t unused) {
    // A credit has arrived - see if we have finished yet.
    checkIfFinished();
  }
  void checkIfFinished() {
    if (this->core->creditsAvailable(this->outChannel) >= (uint)this->operand1)
      this->finishedPhase(this->INST_COMPUTE);
    else
      this->core->waitForCredit(this->outChannel);
  }
};

// Return whether a given input channel has data ready to read.
template<class T>
class TestChannel : public Has1Operand<HasResult<T>> {
public:
  TestChannel(Instruction encoded) : Has1Operand<HasResult<T>>(encoded) {}

  void compute() {
    this->startingPhase(this->INST_COMPUTE);
    this->result = this->core->inputFIFOHasData(this->operand1);
    this->finishedPhase(this->INST_COMPUTE);
  }
  bool computePredicate() {
    return this->result;
  }
};

// Return the index of an input buffer which has data, from those specified in
// a bitmask. If no buffers have data, stall until data arrives.
// The mask's least significant bit represents the first register-mapped FIFO.
template<class T>
class SelectChannel : public Has1Operand<HasResult<T>> {
public:
  SelectChannel(Instruction encoded) : Has1Operand<HasResult<T>>(encoded) {}

  void compute() {
    this->startingPhase(this->INST_COMPUTE);
    this->core->selectChannelWithData(this->operand1);
  }
  void computeCallback(int32_t bufferIndex) {
    this->result = bufferIndex;
    this->finishedPhase(this->INST_COMPUTE);
  }
};

} // end namespace

#endif /* SRC_ISA_OPERATIONS_NETWORK_H_ */
