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

struct StallData {

  // The total number of cycles each core has spent stalled for various
  // reasons.
  CounterMap<ComponentID>
    totalStalls, dataStalls, instructionStalls,
    outputStalls, bypassStalls, idleTimes;

};


class Stalls: public InstrumentationBase {

public:

  // All possible reasons for a core being unable to work. Use a bitmask in
  // case the core stalls for multiple reasons simultaneously.
  enum StallReason {
    NOT_STALLED         = 0,      // Not currently stalled.
    STALL_DATA          = 1 << 0, // Waiting for input data to arrive.
    STALL_INSTRUCTIONS  = 1 << 1, // Waiting for instructions to arrive.
    STALL_OUTPUT        = 1 << 2, // Waiting for output channel to clear.
    STALL_FORWARDING    = 1 << 3, // Waiting for a result to be forwarded.
    IDLE                = 1 << 4  // Have no work to be doing
  };

  static void init();

  static void startLogging();
  static void stopLogging();

  static void startDetailedLog(const string& filename);

  static void stall(const ComponentID id, StallReason reason);
  static void unstall(const ComponentID id, StallReason reason);
  static void activity(const ComponentID id, bool idle);

  static void stall(const ComponentID id, cycle_count_t cycle, StallReason reason);
  static void unstall(const ComponentID id, cycle_count_t cycle, StallReason reason);
  static void idle(const ComponentID id, cycle_count_t cycle);
  static void active(const ComponentID id, cycle_count_t cycle);
  static void endExecution();

  static bool executionFinished();

  static count_t stalledComponents();
  static cycle_count_t cyclesIdle();

  static cycle_count_t cyclesActive(const ComponentID core);
  static cycle_count_t cyclesIdle(const ComponentID core);
  static cycle_count_t cyclesStalled(const ComponentID core);
  static cycle_count_t cyclesLogged();
  static cycle_count_t executionTime();

  static void printStats();
  static void dumpEventCounts(std::ostream& os);

  // A text version of each stall reason.
  static const string name(StallReason reason);

private:

  // Dump detailed information about a particular stall. Information is dumped
  // to the file specified by startDetailedLog.
  static void recordEvent(cycle_count_t currentCycle,
                          ComponentID   component,
                          StallReason   reason,
                          cycle_count_t duration);

  // The reason for each core being stalled at the moment (if stalled).
  static std::map<ComponentID, int> stallReason;

  // Maintain separate logs for normal execution, and for the parts of the
  // program which are explicitly logged.
  static StallData total, loggedOnly;

  // The times that each core started stalling.
  static std::map<ComponentID, cycle_count_t>
    startedStalling, startedDataStall, startedInstructionStall,
    startedOutputStall, startedBypassStall, startedIdle;

  // The number of cores stalled or idle at the moment.
  static count_t numStalled;

  // The cycle number at which all cores became inactive. It is safe to
  // stop simulation if it can also be known that the networks are inactive:
  // it may be the case that a core is waiting for data to arrive.
  static cycle_count_t endOfExecution;

  static cycle_count_t loggedCycles, loggingStarted;

  static bool endExecutionCalled;

  // If the simulator is passed the -stalltrace argument, produce a detailed
  // log of every time a core stalls.
  static bool detailedLog;
  static std::ofstream* logStream;

};

}

#endif /* STALLS_H_ */
