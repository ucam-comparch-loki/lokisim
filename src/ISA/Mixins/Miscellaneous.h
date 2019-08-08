/*
 * Miscellaneous.h
 *
 * Mix-ins which don't fit into other categories.
 *
 *  Created on: 6 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_MIXINS_MISCELLANEOUS_H_
#define SRC_ISA_MIXINS_MISCELLANEOUS_H_

#include <cassert>
#include "../../Datatype/Instruction.h"

// Operations which compute early, usually in the decode stage.
// Redirect the `earlyCompute` methods to access normal `compute` and block
// the normal `compute`.
// This mix-in must wrap any which define `compute()`.
template<class T>
class ComputeEarly : public T {
protected:
  ComputeEarly(Instruction encoded) : T(encoded) {}

  void earlyCompute() {T::compute();}
  void earlyComputeCallback(int32_t value=0) {T::computeCallback(value);}

  void compute() {}
  void computeCallback(int32_t value=0) {assert(false);}
};


#endif /* SRC_ISA_MIXINS_MISCELLANEOUS_H_ */
