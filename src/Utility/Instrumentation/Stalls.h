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
#include <map>

namespace Instrumentation {

class Stalls: public InstrumentationBase {

public:

  static void stall(ComponentID id, int cycle, int reason);
  static void unstall(ComponentID id, int cycle);
  static void idle(ComponentID id, int cycle);
  static void active(ComponentID id, int cycle);
  static void endExecution();
  static void printStats();

  enum StallReason {
    NONE,       // Not currently stalled.
    INPUT,      // Waiting for input to arrive.
    OUTPUT,     // Waiting for output channel to clear.
    PREDICATE   // Waiting to determine predicate's value.
  };

private:

  // The times that each cluster started stalling.
  static std::map<ComponentID, int> startedStalling;

  // The reason for each cluster being stalled at the moment (if stalled).
  static std::map<ComponentID, int> stallReason;

  // The total number of cycles each cluster has spent stalled for various
  // reasons.
  static CounterMap<ComponentID> inputStalls;
  static CounterMap<ComponentID> outputStalls;
  static CounterMap<ComponentID> predicateStalls;

  // The times that each cluster became idle (or UNSTALLED if the cluster is
  // currently active).
  static std::map<ComponentID, int> startedIdle;

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
