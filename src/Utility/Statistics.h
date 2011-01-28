/*
 * Statistics.h
 *
 * Methods to provide access to all information provided by instrumentation.
 *
 * This is separated from the instrumentation itself because it is expected
 * to be quite rare that the same part of the program will both want to store
 * and retrieve data.
 *
 *  Created on: 28 Jan 2011
 *      Author: db434
 */

#ifndef STATISTICS_H_
#define STATISTICS_H_

#include <string>
#include "../Typedefs.h"

class Statistics {

public:

  static void printStats();

  static int getStat(const std::string& statName, int parameter = -1);

  static int executionTime();
  static int energy();

  static int tagChecks();
  static int cacheHits();
  static int cacheMisses();
  static int cacheReads();
  static int cacheWrites();

  static int decodes();

  static int registerReads();
  static int registerWrites();
  static int dataForwards();

  static int operations();
  static int operations(int operation);

  static int memoryReads();
  static int memoryWrites();

  static double networkDistance();

  static int cyclesActive(ComponentID core);
  static int cyclesIdle(ComponentID core);
  static int cyclesStalled(ComponentID core);

};

#endif /* STATISTICS_H_ */
