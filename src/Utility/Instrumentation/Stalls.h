/*
 * Stalls.h
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#ifndef STALLS_H_
#define STALLS_H_

#include "../../Datatype/ComponentID.h"
#include "InstrumentationBase.h"
#include "CounterMap.h"
#include <map>

namespace Instrumentation {

class Stalls: public InstrumentationBase {

public:

  static void stall(const ComponentID& id, unsigned long long cycle, int reason);
  static void unstall(const ComponentID& id, unsigned long long cycle);
  static void idle(const ComponentID& id, unsigned long long cycle);
  static void active(const ComponentID& id, unsigned long long cycle);
  static void endExecution();

  static unsigned long long  cyclesActive(const ComponentID& core);
  static unsigned long long  cyclesIdle(const ComponentID& core);
  static unsigned long long  cyclesStalled(const ComponentID& core);
  static unsigned long long  executionTime();

  static void printStats();

  enum StallReason {
    NOT_STALLED,        // Not currently stalled.
    STALL_DATA,         // Waiting for input data to arrive.
    STALL_INSTRUCTIONS, // Waiting for instructions to arrive.
    STALL_OUTPUT,       // Waiting for output channel to clear.
    STALL_FORWARDING    // Waiting for a result to be forwarded.
  };

private:

  // The times that each cluster started stalling.
  static std::map<ComponentID, unsigned long long> startedStalling;

  // The reason for each cluster being stalled at the moment (if stalled).
  static std::map<ComponentID, int> stallReason;

  // The total number of cycles each cluster has spent stalled for various
  // reasons.
  static CounterMap<ComponentID> dataStalls;
  static CounterMap<ComponentID> instructionStalls;
  static CounterMap<ComponentID> outputStalls;
  static CounterMap<ComponentID> bypassStalls;

  // The times that each cluster became idle (or UNSTALLED if the cluster is
  // currently active).
  static std::map<ComponentID, unsigned long long> startedIdle;

  // The total number of cycles each cluster has spent idle.
  static CounterMap<ComponentID> idleTimes;

  // The number of clusters stalled or idle at the moment.
  static uint16_t numStalled;

  // The cycle number at which all clusters became inactive. It is safe to
  // stop simulation if it can also be known that the networks are inactive:
  // it may be the case that a cluster is waiting for data to arrive.
  static uint32_t endOfExecution;

};

}

#endif /* STALLS_H_ */
