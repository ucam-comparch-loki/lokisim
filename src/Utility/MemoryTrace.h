/*
 * Instrumentation.h
 *
 * Record statistics on runtime behaviour, so they can be collated and
 * summarised when execution completes.
 *
 * To extract the statistics, use the Statistics class.
 *
 *  Created on: 16 Jun 2010
 *      Author: db434
 */

#ifndef MEMORYTRACE_H_
#define MEMORYTRACE_H_

#include <string>

#include "Parameters.h"

namespace MemoryTrace {

  // Initialize memory tracing engine
  void start(const std::string& filename);

  // Update clock cycle
  void setClockCycle(unsigned long long cycleNumber);

  // Log memory accesses
  void readIPKWord(unsigned long bankNumber, unsigned long address);
  void readWord(unsigned long bankNumber, unsigned long address);
  void readHalfWord(unsigned long bankNumber, unsigned long address);
  void readByte(unsigned long bankNumber, unsigned long address);
  void writeWord(unsigned long bankNumber, unsigned long address);
  void writeHalfWord(unsigned long bankNumber, unsigned long address);
  void writeByte(unsigned long bankNumber, unsigned long address);

  // Finalize memory trace
  void stop();

}

#endif /* MEMORYTRACE_H_ */
