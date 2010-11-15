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

namespace Instrumentation {

class Operations: public InstrumentationBase {

public:

  static void operation(int op, bool executed);
  static void printStats();

private:

  static CounterMap<int> executedOps;
  static CounterMap<int> unexecutedOps;
  static int numOps;

};

}

#endif /* OPERATIONS_H_ */
