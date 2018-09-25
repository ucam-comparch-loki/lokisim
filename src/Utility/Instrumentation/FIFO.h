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

  // Record whenever data is added to or removed from the FIFO. size is the
  // total number of elements the FIFO can hold. (Include bit width too?)
  static void push(size_t size);
  static void pop(size_t size);

  static void createdFIFO(size_t size);
  static void activeCycle(size_t size);

  static void dumpEventCounts(std::ostream& os, const chip_parameters_t& params);

private:

  // Record how many pushes and pops there were for FIFOs of each size.
  static CounterMap<size_t> pushes, pops;

  // Count how many FIFOs of each size there are so we can compute leakage.
  static CounterMap<size_t> instances;

  // Count how many cycles each size of FIFO is active.
  static CounterMap<size_t> activeCycles;

};

}

#endif /* FIFO_H_ */
