/*
 * CoreTrace.h
 *
 *  Created on: 28 Sep 2011
 *      Author: afjk2
 */

#ifndef CORETRACE_H_
#define CORETRACE_H_

#include <string>

#include "../Parameters.h"

namespace CoreTrace {

  // Initialize memory tracing engine
  void start(const std::string& filename);

  // Update clock cycle
  void setClockCycle(unsigned long long cycleNumber);

  // Log core activity
  void decodeInstruction(unsigned long coreNumber, unsigned long address, bool endOfPacket);

  // Finalize core trace
  void stop();

}

#endif /* CORETRACE_H_ */
