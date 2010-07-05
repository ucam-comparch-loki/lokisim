/*
 * Stalls.h
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#ifndef STALLS_H_
#define STALLS_H_

#include "InstrumentationBase.h"
#include "CounterMap.h"

class Stalls: public InstrumentationBase {

public:

  static void stall(int id, int cycle);
  static void unstall(int id, int cycle);
  static void idle(int id, int cycle);
  static void active(int id, int cycle);
  static void printStats();

private:

  // The times that each cluster started stalling (or UNSTALLED if the cluster
  // is currently active).
  static CounterMap<int> stalled;

  // The total number of cycles each cluster has spent stalled.
  static CounterMap<int> stallTimes;

  // The times that each cluster became idle (or UNSTALLED if the cluster is
  // currently active).
  static CounterMap<int> idleStart;

  // The total number of cycles each cluster has spent idle.
  static CounterMap<int> idleTimes;

  // The number of clusters stalled or idle at the moment.
  static int numStalled;

  // The cycle number at which all clusters became inactive. It is safe to
  // stop simulation if it can also be known that the networks are inactive:
  // it may be the case that a cluster is waiting for data to arrive.
  static int endOfExecution;

};

#endif /* STALLS_H_ */
