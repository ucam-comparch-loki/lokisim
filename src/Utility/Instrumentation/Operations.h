/*
 * Operations.h
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#ifndef OPERATIONS_H_
#define OPERATIONS_H_

#include <vector>
#include "InstrumentationBase.h"
#include "CounterMap.h"
#include "../../Datatype/Identifier.h"
#include "../../Datatype/DecodedInst.h"

using std::vector;

namespace Instrumentation {

class Operations: public InstrumentationBase {

public:

  static void init(const chip_parameters_t& params);
  static void reset();

  static void decoded(const ComponentID& core, const DecodedInst& dec);
  static void executed(const Core& core, const DecodedInst& dec, bool executed);
  static void acceleratorTick(const ComponentID& acc, uint numOps);

  static count_t numDecodes();
  static count_t numOperations();
  static count_t numOperations(const ComponentID& core);
  static count_t numOperations(opcode_t operation,
                               function_t function = (function_t)0);

  // A second operation count which continues to increase, even when statistics
  // aren't being collected. Used for debug purposes.
  static count_t allOperations();

  static void printStats();
  static void printSummary(const chip_parameters_t& params);
  static void dumpEventCounts(std::ostream& os, const chip_parameters_t& params);

  static CounterMap<ComponentID> numMemLoads;
  static CounterMap<ComponentID> numMergedMemLoads;
  static CounterMap<ComponentID> numMemStores;
  static CounterMap<ComponentID> numChanReads;
  static CounterMap<ComponentID> numMergedChanReads; // i.e. packed with a useful instruction
  static CounterMap<ComponentID> numChanWrites;
  static CounterMap<ComponentID> numMergedChanWrites;
  static CounterMap<ComponentID> numArithOps;
  static CounterMap<ComponentID> numCondOps;

private:

  static CounterMap<opcode_t> executedOps;
  static CounterMap<function_t> executedFns;
  static count_t unexecuted;

  // Store the previous inputs, outputs, and operations seen, so that we can
  // compare them to the latest values.
  static vector<int32_t> lastIn1, lastIn2, lastOut;
  static vector<function_t> lastFn;

  // Counters used to help compute energy consumption. hd = Hamming distance.
  static count_t hdIn1, hdIn2, hdOut, sameOp;

  // Is there a difference between numOps and numDecodes?
  static CounterMap<ComponentID> numOps_;
  static count_t numDecodes_;

};

}

#endif /* OPERATIONS_H_ */
