/*
 * Instrumentation.h
 *
 * Record statistics on runtime behaviour, so they can be collated and
 * summarised when execution completes.
 *
 *  Created on: 16 Jun 2010
 *      Author: db434
 */

#ifndef INSTRUMENTATION_H_
#define INSTRUMENTATION_H_

#include <iostream>

class Instruction;

class Instrumentation {

public:

  // Record whether there was a cache hit or miss.
  static void IPKCacheHit(bool hit);

  // Record that memory was read from.
  static void memoryRead();

  // Record that memory was written to.
  static void memoryWrite();

  // Record that a particular core stalled or unstalled.
  static void stalled(int id, bool stalled, int cycle);

  // Record that a particular core became idle or active.
  static void idle(int id, bool idle, int cycle);

  // End execution immediately.
  static void endExecution();

  // Record that data was sent over the network.
  static void networkTraffic(int startID, int endID, double distance);

  // Record whether a particular operation was executed or not.
  static void operation(Instruction inst, bool executed, int id);

  // Print the results of instrumentation.
  static void printStats();

};

#endif /* INSTRUMENTATION_H_ */
