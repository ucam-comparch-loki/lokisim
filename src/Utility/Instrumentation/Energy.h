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
  static unsigned long long totalEnergy();

  // Picojoules used per operation, on average: our chosen metric. Target = 10.
  // Only takes into account operations which were executed; those which were
  // fetched/decoded but not used aren't counted.
  static double pJPerOp();

  static void printStats();

private:

  static unsigned long long registerEnergy();
  static unsigned long long cacheEnergy();
  static unsigned long long memoryEnergy();
  static unsigned long long decodeEnergy();
  static unsigned long long operationEnergy();
  static unsigned long long networkEnergy();

  static bool libraryLoaded;

  // Costs of various actions, in femtojoules.
  static int  regRead, regWrite, stallReg;
  static int  l0Read,  l0Write,  l0TagCheck;
  static int  l1Read,  l1Write,  l1TagCheck;
  static int  decode;
  static int  op, multiply;
  static int  bitMillimetre;

};

}

#endif /* ENERGY_H_ */
