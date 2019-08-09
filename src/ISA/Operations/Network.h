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

// Wait on channel end. Block the pipeline until the given output channel
// has at least a required number of credits.
template<class T>
class WaitOnChannelEnd : public T {
public:
  WaitOnChannelEnd(Instruction encoded) : T(encoded) {}

  void compute() {
    if (this->core->creditsAvailable(this->outChannel) >= this->immediate)
      this->finished.notify(sc_core::SC_ZERO_TIME);

    // TODO: else wait for this->core->creditArrivedEvent(this->outChannel)
  }
  void computeCallback(int32_t unused) {
    // A credit has arrived - see if we have finished yet.
    compute();
  }
};

// Return whether a given input channel has data ready to read.
template<class T>
class TestChannel : public T {
public:
  TestChannel(Instruction encoded) : T(encoded) {}

  void compute() {
    this->result = this->core->inputFIFOHasData(this->operand1);
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  bool computePredicate() {
    return this->result;
  }
};

// Return the index of an input buffer which has data, from those specified in
// a bitmask. If no buffers have data, stall until data arrives.
// The mask's least significant bit represents the first register-mapped FIFO.
template<class T>
class SelectChannel : public T {
public:
  SelectChannel(Instruction encoded) : T(encoded) {}

  void compute() {
    this->core->selectChannelWithData(this->operand1);
  }
  void computeCallback(int32_t bufferIndex) {
    this->result = bufferIndex;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};

#endif /* SRC_ISA_OPERATIONS_NETWORK_H_ */
