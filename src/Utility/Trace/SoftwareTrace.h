/*
 * SoftwareTrace.h
 *
 *  Created on: 11 Jan 2013
 *      Author: afjk2
 */

#ifndef SOFTWARETRACE_H_
#define SOFTWARETRACE_H_

#include <string>

#include "../Parameters.h"

namespace SoftwareTrace {

  // Initialize software tracing engine
  void start(const std::string& filename);

  // Update clock cycle
  void setClockCycle(unsigned long long cycleNumber);

  // Log register file snapshot when requested by syscall
  void logRegisterFileSnapshot(unsigned long coreNumber, unsigned long data[], unsigned long count);

  // Finalize core trace
  void stop();

}

#endif /* SOFTWARETRACE_H_ */
