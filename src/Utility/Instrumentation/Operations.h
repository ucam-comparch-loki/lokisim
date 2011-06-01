/*
 * Operations.h
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#ifndef OPERATIONS_H_
#define OPERATIONS_H_

#include "../../Datatype/ComponentID.h"
#include "InstrumentationBase.h"
#include "CounterMap.h"

class DecodedInst;

namespace Instrumentation {

class Operations: public InstrumentationBase {

public:

  static void decoded(const ComponentID &core, const DecodedInst& dec);
  static void operation(int op, bool executed);

  static unsigned long long  numDecodes();
  static unsigned long long  numOperations();
  static unsigned long long  numOperations(int operation);

  static void printStats();

private:

  static CounterMap<int> executedOps;
  static CounterMap<int> unexecutedOps;

  // Is there a difference between numOps and numDecodes?
  static unsigned long long numOps_, numDecodes_;

};

}

#endif /* OPERATIONS_H_ */
