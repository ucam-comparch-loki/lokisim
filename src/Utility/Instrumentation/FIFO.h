/*
 * FIFO.h
 *
 *  Created on: 2 May 2012
 *      Author: db434
 */

#ifndef FIFO_H_
#define FIFO_H_

#include "InstrumentationBase.h"
#include "CounterMap.h"

namespace Instrumentation {

class FIFO : public InstrumentationBase {

public:

  static void init();
  static void end();

  // Record whenever data is added to or removed from the FIFO. size is the
  // total number of elements the FIFO can hold. (Include bit width too?)
  static void push(size_t size);
  static void pop(size_t size);

  static void dumpEventCounts(std::ostream& os);

private:

  // Record how many pushes and pops there were for FIFOs of each size.
  static CounterMap<size_t> pushes, pops;

};

}

#endif /* FIFO_H_ */
