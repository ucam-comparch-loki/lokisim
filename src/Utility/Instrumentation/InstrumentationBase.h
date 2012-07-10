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

class AddressedWord;

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
  static int popcount(const AddressedWord& value);
  static int hammingDistance(uint val1, uint val2);
  static int hammingDistance(const AddressedWord& val1, const AddressedWord& val2);

  static string percentage(count_t value, count_t total);

  // Produce a line of the form: <name>
  static const string xmlBegin(const char* name);

  // Produce a line of the form: </name>
  static const string xmlEnd(const char* name);

  // Produce a line of the form: <name>value</name>
  static const string xmlNode(const char* name, count_t value, const char* indent="\t");

};

}

#endif /* INSTRUMENTATIONBASE_H_ */
