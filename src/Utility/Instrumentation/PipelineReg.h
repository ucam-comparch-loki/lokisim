/*
 * PipelineReg.h
 *
 *  Created on: 3 Jul 2012
 *      Author: db434
 */

#ifndef PIPELINEREG_H_
#define PIPELINEREG_H_

#include "../../Tile/Core/PipelineRegister.h"
#include "InstrumentationBase.h"
#include "CounterMap.h"

class DecodedInst;

namespace Instrumentation {

class PipelineReg: public InstrumentationBase {

public:

  // Record when new data was written to a pipeline register. Also keep track
  // of which stage in the pipeline we're at - only interested in switching in
  // a subset of the instruction's data.
  static void activity(const DecodedInst& oldVal,
                       const DecodedInst& newVal,
                       PipelineRegister::PipelinePosition stage);

  static void dumpEventCounts(std::ostream& os);

private:

  // Keep track of the number of cycles each pipeline register is active, and
  // the number of bits toggled.
  static CounterMap<size_t> active, hammingDist;

};

}

#endif /* PIPELINEREG_H_ */
