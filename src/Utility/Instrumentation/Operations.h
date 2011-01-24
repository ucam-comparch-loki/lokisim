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

class DecodedInst;

namespace Instrumentation {

class Operations: public InstrumentationBase {

public:

  static void decoded(ComponentID core, const DecodedInst& dec);
  static void operation(int op, bool executed);
  static void printStats();

private:

  static CounterMap<int> executedOps;
  static CounterMap<int> unexecutedOps;

  // Is there a difference between numOps and numDecodes?
  static int numOps, numDecodes;

};

}

#endif /* OPERATIONS_H_ */
