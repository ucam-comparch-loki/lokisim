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

namespace Instrumentation {

class Stalls: public InstrumentationBase {

public:

  static void stall(ComponentID id, int cycle);
  static void unstall(ComponentID id, int cycle);
  static void idle(ComponentID id, int cycle);
  static void active(ComponentID id, int cycle);
  static void endExecution();
  static void printStats();

private:

  // The times that each cluster started stalling (or UNSTALLED if the cluster
  // is currently active).
  static CounterMap<ComponentID> stalled;

  // The total number of cycles each cluster has spent stalled.
  static CounterMap<ComponentID> stallTimes;

  // The times that each cluster became idle (or UNSTALLED if the cluster is
  // currently active).
  static CounterMap<ComponentID> idleStart;

  // The total number of cycles each cluster has spent idle.
  static CounterMap<ComponentID> idleTimes;

  // The number of clusters stalled or idle at the moment.
  static int numStalled;

  // The cycle number at which all clusters became inactive. It is safe to
  // stop simulation if it can also be known that the networks are inactive:
  // it may be the case that a cluster is waiting for data to arrive.
  static int endOfExecution;

};

}

#endif /* STALLS_H_ */
