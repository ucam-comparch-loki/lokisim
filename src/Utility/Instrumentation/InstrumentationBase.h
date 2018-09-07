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

#include "../../Network/NetworkTypes.h"
#include "../Instrumentation.h"
#include "../../Types.h"
#include "../../Utility/Arguments.h"

using std::string;
using std::cout;
using std::endl;

// Allow ComponentIDs to be compared so they can be used in lookup tables.
namespace std {
  template<>
  struct less<ComponentID> {
    bool operator() (const ComponentID& lhs, const ComponentID& rhs) const {
      return lhs.flatten() < rhs.flatten();
    }
  };
}

namespace Instrumentation {

class InstrumentationBase {

public:

  static void printStats();

  // Perform any necessary initialisation before data collection begins.
  static void init(const chip_parameters_t& params);

  // Reset any statistics collected so far.
  static void reset();

  // Start collecting data.
  static void start();

  // Stop collecting data.
  static void stop();

  // Tidy up after all data has been retrieved.
  static void end();

protected:

  static int popcount(uint value);
  static int popcount(const NetworkData& value);
  static int hammingDistance(uint val1, uint val2);
  static int hammingDistance(const NetworkData& val1, const NetworkData& val2);

  static string percentage(count_t value, count_t total);

  // Produce a line of the form: <name>
  static const string xmlBegin(const char* name);

  // Produce a line of the form: </name>
  static const string xmlEnd(const char* name);

  // Produce a line of the form: <name>value</name>
  static const string xmlNode(const char* name, count_t value, const char* indent="\t");

  static chip_parameters_t& params;

};

}

#endif /* INSTRUMENTATIONBASE_H_ */
