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

using std::string;
using std::cout;
using std::endl;

class InstrumentationBase {

public:

  static void printStats();

protected:

  static string asPercentage(int value, int total);

};

#endif /* INSTRUMENTATIONBASE_H_ */