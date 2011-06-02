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
#include "../Datatype/ComponentID.h"
#include "../Typedefs.h"

class Statistics {

public:

  static void printStats();

  static int getStat(const std::string& statName, int parameter = -1);

  static int executionTime();
  static int energy();
  static int fJPerOp();

  static int decodes();

  static int operations();
  static int operations(int operation);

  static int registerReads();
  static int registerWrites();
  static int dataForwards();
  static int stallRegUses();

  static int l0TagChecks();
  static int l0Hits();
  static int l0Misses();
  static int l0Reads();
  static int l0Writes();

  static int l1TagChecks();
  static int l1Hits();
  static int l1Misses();
  static int l1Reads();
  static int l1Writes();

  static double networkDistance();

  static int cyclesActive(const ComponentID& core);
  static int cyclesIdle(const ComponentID& core);
  static int cyclesStalled(const ComponentID& core);

};

#endif /* STATISTICS_H_ */
