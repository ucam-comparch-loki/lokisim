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

  static int  numDecodes();
  static int  numOperations();
  static int  numOperations(int operation);

  static void printStats();

private:

  static CounterMap<int> executedOps;
  static CounterMap<int> unexecutedOps;

  // Is there a difference between numOps and numDecodes?
  static int numOps_, numDecodes_;

};

}

#endif /* OPERATIONS_H_ */
