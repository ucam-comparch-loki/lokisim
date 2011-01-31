/*
 * Energy.h
 *
 *  Created on: 31 Jan 2011
 *      Author: db434
 */

#ifndef ENERGY_H_
#define ENERGY_H_

#include "InstrumentationBase.h"
#include <string>

namespace Instrumentation {

class Energy : public InstrumentationBase {

public:

  static void loadLibrary(const std::string& filename);

  // Total energy consumed, in femtojoules.
  static long totalEnergy();

  // Picojoules used per operation, on average: our chosen metric. Target = 10.
  static double pJPerOp();

  static void printStats();

private:

  static long registerEnergy();
  static long cacheEnergy();
  static long memoryEnergy();
  static long operationEnergy();
  static long networkEnergy();

  static bool libraryLoaded;

};

}

#endif /* ENERGY_H_ */
