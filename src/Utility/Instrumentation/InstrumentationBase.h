/*
 * InstrumentationBase.h
 *
 * The base class for all instrumentation classes. Specifies that all classes
 * should be able to print statistics, and provides a method for consistently
 * computing percentages.
 *
 *  Created on: 16 Jun 2010
 *      Author: db434
 */

#ifndef INSTRUMENTATIONBASE_H_
#define INSTRUMENTATIONBASE_H_

#include <iostream>
#include "../../Typedefs.h"

using std::string;
using std::cout;
using std::endl;

typedef unsigned long long count_t;

namespace Instrumentation {

class InstrumentationBase {

public:

  static void printStats();

  // Perform any necessary initialisation before data collection begins.
  static void init();

  // Tidy up after all data has been retrieved.
  static void end();

protected:

  static int popcount(uint value);
  static int hammingDistance(uint val1, uint val2);

  static string percentage(count_t value, count_t total);

};

}

using namespace Instrumentation;

#endif /* INSTRUMENTATIONBASE_H_ */
