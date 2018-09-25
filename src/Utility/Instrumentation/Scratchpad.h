/*
 * Scratchpad.h
 *
 *  Created on: 17 Jan 2013
 *      Author: db434
 */

#ifndef SCRATCHPAD_INSTRUMENTATION_H_
#define SCRATCHPAD_INSTRUMENTATION_H_

#include "InstrumentationBase.h"

namespace Instrumentation {

class Scratchpad : public InstrumentationBase {

public:

  static void read();
  static void write();

  static count_t numReads();
  static count_t numWrites();

  static void dumpEventCounts(std::ostream& os, const chip_parameters_t& params);

private:

  static count_t reads, writes;

};

}

#endif /* SCRATCHPAD_INSTRUMENTATION_H_ */
