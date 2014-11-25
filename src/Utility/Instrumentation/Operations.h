/*
 * Operations.h
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#ifndef OPERATIONS_H_
#define OPERATIONS_H_

#include "InstrumentationBase.h"
#include "CounterMap.h"
#include "../../Datatype/ComponentID.h"
#include "../../Datatype/DecodedInst.h"

namespace Instrumentation {

class Operations: public InstrumentationBase {

public:

  static void init();
  static void end();

  static void decoded(const ComponentID& core, const DecodedInst& dec);
  static void executed(const ComponentID& core, const DecodedInst& dec, bool executed);

  static count_t numDecodes();
  static count_t numOperations();
  static count_t numOperations(const ComponentID& core);
  static count_t numOperations(opcode_t operation,
                               function_t function = (function_t)0);

  static void printStats();
  static void printSummary();
  static void dumpEventCounts(std::ostream& os);

private:

  static CounterMap<opcode_t> executedOps;
  static CounterMap<function_t> executedFns;
  static count_t unexecuted;

  // Store the previous inputs, outputs, and operations seen, so that we can
  // compare them to the latest values.
  static int32_t *lastIn1, *lastIn2, *lastOut;
  static function_t *lastFn;

  // Counters used to help compute energy consumption. hd = Hamming distance.
  static count_t hdIn1, hdIn2, hdOut, sameOp;

  // Is there a difference between numOps and numDecodes?
  static CounterMap<ComponentID> numOps_;
  static count_t numDecodes_;

};

}

#endif /* OPERATIONS_H_ */
