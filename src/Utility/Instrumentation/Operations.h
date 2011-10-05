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

  static void decoded(const ComponentID& core, const DecodedInst& dec);
  static void operation(opcode_t op, bool executed);

  static unsigned long long numDecodes();
  static unsigned long long numOperations();
  static unsigned long long numOperations(opcode_t operation);

  static void printStats();

private:

  static CounterMap<opcode_t> executedOps;
  static CounterMap<opcode_t> unexecutedOps;

  // Is there a difference between numOps and numDecodes?
  static unsigned long long numOps_, numDecodes_;

};

}

#endif /* OPERATIONS_H_ */
