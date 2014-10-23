/*
 * Stalls.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include <systemc>
#include "Stalls.h"
#include "Operations.h"
#include "../../Datatype/ComponentID.h"
#include "../../Exceptions/InvalidOptionException.h"
#include "../Instrumentation.h"
#include "../Parameters.h"

using namespace Instrumentation;

// If a core is not currently stalled, it should map to this value in the
// "stalled" mapping.
const cycle_count_t UNSTALLED = -1;

const cycle_count_t NOT_LOGGING = -1;

std::map<ComponentID, int> Stalls::stallReason;

std::map<ComponentID, cycle_count_t> Stalls::startedStalling;
std::map<ComponentID, cycle_count_t> Stalls::startedDataStall;
std::map<ComponentID, cycle_count_t> Stalls::startedInstructionStall;
std::map<ComponentID, cycle_count_t> Stalls::startedOutputStall;
std::map<ComponentID, cycle_count_t> Stalls::startedBypassStall;
std::map<ComponentID, cycle_count_t> Stalls::startedIdle;

StallData Stalls::total;
StallData Stalls::loggedOnly;

count_t Stalls::numStalled = 0;
cycle_count_t Stalls::endOfExecution = 0;
cycle_count_t Stalls::loggedCycles = 0;
cycle_count_t Stalls::loggingStarted = NOT_LOGGING;
bool Stalls::endExecutionCalled = false;

bool Stalls::detailedLog = false;
std::ofstream* Stalls::logStream;

void Stalls::init() {
  for (uint i=0; i<NUM_TILES; i++) {
    for (uint j=0; j<COMPONENTS_PER_TILE; j++) {
      ComponentID id(i, j);

      stallReason[id]             = IDLE;
      numStalled++;

      startedStalling[id]         = 0;
      startedDataStall[id]        = UNSTALLED;
      startedInstructionStall[id] = UNSTALLED;
      startedOutputStall[id]      = UNSTALLED;
      startedBypassStall[id]      = UNSTALLED;
      startedIdle[id]             = 0;
    }
  }
}

void Stalls::startLogging() {
  // FIXME: what if execution hasn't started yet, and the current cycle is
  // undefined?
  loggingStarted = currentCycle();

  // If any cores are stalled, pretend that they only just started, so cycles
  // before logging started aren't counted.
  for (uint i=0; i<NUM_TILES; i++) {
    for (uint j=0; j<COMPONENTS_PER_TILE; j++) {
      ComponentID id(i, j);

      if (startedStalling[id]         != UNSTALLED)
        startedStalling[id]         = loggingStarted;
      if (startedDataStall[id]        != UNSTALLED)
        startedDataStall[id]        = loggingStarted;
      if (startedInstructionStall[id] != UNSTALLED)
        startedInstructionStall[id] = loggingStarted;
      if (startedOutputStall[id]      != UNSTALLED)
        startedOutputStall[id]      = loggingStarted;
      if (startedBypassStall[id]      != UNSTALLED)
        startedBypassStall[id]      = loggingStarted;
      if (startedIdle[id]             != UNSTALLED)
        startedIdle[id]             = loggingStarted;
    }
  }
}

void Stalls::stopLogging() {
  //if (loggingStarted == NOT_LOGGING)
  //  return;

  if (loggingStarted != NOT_LOGGING)
    loggedCycles += currentCycle() - loggingStarted;

  // Unstall all cores so their stall durations are added to the totals.
  for (uint i=0; i<NUM_TILES; i++) {
    for (uint j=0; j<COMPONENTS_PER_TILE; j++) {
      ComponentID id(i, j);

      if (startedDataStall[id] != UNSTALLED) {
        unstall(id, STALL_DATA);
        stall(id, STALL_DATA);
      }
      if (startedInstructionStall[id] != UNSTALLED) {
        unstall(id, STALL_INSTRUCTIONS);
        stall(id, STALL_INSTRUCTIONS);
      }
      if (startedOutputStall[id] != UNSTALLED) {
        unstall(id, STALL_OUTPUT);
        stall(id, STALL_OUTPUT);
      }
      if (startedBypassStall[id] != UNSTALLED) {
        unstall(id, STALL_FORWARDING);
        stall(id, STALL_FORWARDING);
      }
      if (startedIdle[id] != UNSTALLED) {
        unstall(id, IDLE);
        stall(id, IDLE);
      }
    }
  }

  loggingStarted = NOT_LOGGING;

  delete logStream;
}

void Stalls::startDetailedLog(const string& filename) {
  detailedLog = true;
  logStream = new std::ofstream(filename.c_str());

  // Print header line.
  logStream->width(12);
  *logStream << "cycle";
  logStream->width(12);
  *logStream << "core";
  logStream->width(12);
  *logStream << "reason";
  logStream->width(12);
  *logStream << "duration" << endl;
}

void Stalls::stall(const ComponentID id, StallReason reason) {
  Stalls::stall(id, currentCycle(), reason);
}

void Stalls::unstall(const ComponentID id, StallReason reason) {
  Stalls::unstall(id, currentCycle(), reason);
}

void Stalls::activity(const ComponentID id, bool idle) {
  if (idle) Stalls::idle(id, currentCycle());
  else Stalls::active(id, currentCycle());
}

void Stalls::stall(const ComponentID id, cycle_count_t cycle, StallReason reason) {
  // We're already stalled for this reason.
  if (stallReason[id] & reason)
    return;

  // We can't become idle if we're already stalled.
  if ((reason == IDLE) && (stallReason[id] != NOT_STALLED))
    return;

  // If we are stalled, we have work to do, so can't be idle.
  if (stallReason[id] & IDLE)
    unstall(id, cycle, IDLE);

  if (stallReason[id] == NOT_STALLED) {
    numStalled++;
    assert(numStalled <= NUM_COMPONENTS);

    startedStalling[id] = cycle;

    if (numStalled >= NUM_COMPONENTS)
      endOfExecution = cycle;
  }

  stallReason[id] |= reason;

  switch (reason) {
    case STALL_DATA:         startedDataStall[id] = cycle;        break;
    case STALL_INSTRUCTIONS: startedInstructionStall[id] = cycle; break;
    case STALL_OUTPUT:       startedOutputStall[id] = cycle;      break;
    case STALL_FORWARDING:   startedBypassStall[id] = cycle;      break;
    case IDLE:               startedIdle[id] = cycle;             break;

    default: std::cerr << "Warning: Unknown stall reason." << endl; break;
  }

}

cycle_count_t max(cycle_count_t time1, cycle_count_t time2) {
  return (time1 > time2) ? time1 : time2;
}

void Stalls::unstall(const ComponentID id, cycle_count_t cycle, StallReason reason) {
  if (stallReason[id] & reason) {
    cycle_count_t timeStalled = 0;

    switch (reason) {
      case STALL_DATA:
        if (ENERGY_TRACE)
          loggedOnly.dataStalls.increment(id, cycle - max(loggingStarted, startedDataStall[id]));
        timeStalled = cycle - startedDataStall[id];
        total.dataStalls.increment(id, timeStalled);
        startedDataStall[id] = UNSTALLED;
        break;

      case STALL_INSTRUCTIONS:
        if (ENERGY_TRACE)
          loggedOnly.instructionStalls.increment(id, cycle - max(loggingStarted, startedInstructionStall[id]));
        timeStalled = cycle - startedInstructionStall[id];
        total.instructionStalls.increment(id, timeStalled);
        startedInstructionStall[id] = UNSTALLED;
        break;

      case STALL_OUTPUT:
        if (ENERGY_TRACE)
          loggedOnly.outputStalls.increment(id, cycle - max(loggingStarted, startedOutputStall[id]));
        timeStalled = cycle - startedOutputStall[id];
        total.outputStalls.increment(id, timeStalled);
        startedOutputStall[id] = UNSTALLED;
        break;

      case STALL_FORWARDING:
        if (ENERGY_TRACE)
          loggedOnly.bypassStalls.increment(id, cycle - max(loggingStarted, startedBypassStall[id]));
        timeStalled = cycle - startedBypassStall[id];
        total.bypassStalls.increment(id, timeStalled);
        startedBypassStall[id] = UNSTALLED;
        break;

      case IDLE:
        if (ENERGY_TRACE)
          loggedOnly.idleTimes.increment(id, cycle - max(loggingStarted, startedIdle[id]));
        timeStalled = cycle - startedIdle[id];
        total.idleTimes.increment(id, timeStalled);
        startedIdle[id] = UNSTALLED;
        break;

      case NOT_STALLED:
        break;
    }

    // Clear this stall reason from the bitmask.
    stallReason[id] &= ~reason;

    if (detailedLog)
      recordEvent(cycle, id, reason, timeStalled);

    if (stallReason[id] == NOT_STALLED) {
      numStalled--;
      assert(numStalled <= NUM_COMPONENTS);

      if (ENERGY_TRACE)
        loggedOnly.totalStalls.increment(id, cycle - max(loggingStarted, startedStalling[id]));
      total.totalStalls.increment(id, cycle - startedStalling[id]);
      startedStalling[id] = UNSTALLED;
    }
  }
}

void Stalls::idle(const ComponentID id, cycle_count_t cycle) {
  stall(id, cycle, IDLE);
}

void Stalls::active(const ComponentID id, cycle_count_t cycle) {
  // Only become active if we were idle. If we were properly stalled, waiting
  // for information, then we are still stalled.
//  if (stallReason[id] & IDLE)
    unstall(id, cycle, IDLE);
}

void Stalls::endExecution() {
  if (numStalled < NUM_COMPONENTS)
    endOfExecution = sc_core::sc_time_stamp().to_default_time_units();

  endExecutionCalled = true;
}

bool Stalls::executionFinished() {
  return endExecutionCalled;
}

count_t Stalls::stalledComponents() {
  return numStalled;
}

cycle_count_t Stalls::cyclesIdle() {
  if (stalledComponents() == NUM_COMPONENTS)
    return currentCycle() - endOfExecution;
  else
    return 0;
}

cycle_count_t Stalls::cyclesActive(const ComponentID core) {
  return currentCycle() - cyclesStalled(core) - cyclesIdle(core);
}

cycle_count_t Stalls::cyclesIdle(const ComponentID core) {
  return total.idleTimes[core];
}

cycle_count_t Stalls::cyclesStalled(const ComponentID core) {
  return total.totalStalls[core] - cyclesIdle(core);
}

cycle_count_t Stalls::cyclesLogged() {
  return loggedCycles;
}

cycle_count_t Stalls::executionTime() {
  return endOfExecution;
}

void Stalls::printStats() {
  //TODO: Add information to database if required

  using std::clog;

  if (endOfExecution == 0) return;

  clog << "Core activity:" << endl;
  clog << "  Core\tInstructions\tActive\tIdle\tStalled (insts|data|bypass|output)" << endl;

  // Flush any remaining stall/idle time into the CounterMaps.
  stopLogging();

	for (uint i=0; i<NUM_TILES; i++) {
		for (uint j=0; j<COMPONENTS_PER_TILE; j++) {
			ComponentID id(i, j);

			// Skip over memories for now -- they are not instrumented properly.
			if (id.isMemory()) continue;

			// Only print statistics for cores which have seen some activity.
			if ((uint)total.idleTimes[id] < endOfExecution) {
				cycle_count_t totalStalled = cyclesStalled(id);
				cycle_count_t activeCycles = cyclesActive(id);
				clog << "  " << id << "\t" <<
				Operations::numOperations(id) << "\t\t" <<
				percentage(activeCycles, endOfExecution) << "\t" <<
				percentage(total.idleTimes[id], endOfExecution) << "\t" <<
				percentage(totalStalled, endOfExecution) << "\t(" <<
        percentage(total.instructionStalls[id], totalStalled) << "|" <<
        percentage(total.dataStalls[id], totalStalled) << "|" <<
				percentage(total.bypassStalls[id], totalStalled) << "|" <<
				percentage(total.outputStalls[id], totalStalled) << ")" << endl;
			}
		}
	}

  if (BATCH_MODE)
	cout << "<@GLOBAL>total_cycles:" << endOfExecution << "</@GLOBAL>" << endl;

  clog << "Total execution time: " << endOfExecution << " cycles" << endl;

}

void Stalls::dumpEventCounts(std::ostream& os) {
  for (uint i=0; i<NUM_TILES; i++) {
    for (uint j=0; j<COMPONENTS_PER_TILE; j++) {
      ComponentID id(i, j);

      // Only interested in cores.
      if (id.isMemory())
        continue;

      // Flush any remaining stall/idle time into the CounterMaps.
//      unstall(id, endOfExecution, IDLE);
//      active(id, endOfExecution);
      stopLogging();

      // Skip over any cores which were idle the whole time.
//      if ((cycle_count_t)idleTimes[id] >= endOfExecution)
//        continue;

      cycle_count_t totalStalled = loggedOnly.totalStalls[id] - loggedOnly.idleTimes[id];
      cycle_count_t activeCycles = loggedCycles - loggedOnly.totalStalls[id];

      os << "<activity core=\"" << id.getGlobalCoreNumber()  << "\">\n"
         << xmlNode("active", activeCycles)                     << "\n"
         << xmlNode("idle", loggedOnly.idleTimes[id])                      << "\n"
         << xmlNode("stalled", totalStalled)                    << "\n"
         << xmlNode("instruction_stall", loggedOnly.instructionStalls[id]) << "\n"
         << xmlNode("data_stall", loggedOnly.dataStalls[id])               << "\n"
         << xmlNode("bypass_stall", loggedOnly.bypassStalls[id])           << "\n"
         << xmlNode("output_stall", loggedOnly.outputStalls[id])           << "\n"
         << xmlEnd("activity")                                  << "\n";
    }
  }
}

const string Stalls::name(StallReason reason) {
  switch (reason) {
    case NOT_STALLED:         return "not stalled";
    case STALL_DATA:          return "data";
    case STALL_INSTRUCTIONS:  return "inst";
    case STALL_OUTPUT:        return "output";
    case STALL_FORWARDING:    return "forwarding";
    case IDLE:                return "idle";
    default: throw InvalidOptionException("stall reason", reason);
  }
}

void Stalls::recordEvent(cycle_count_t currentCycle,
                         ComponentID   component,
                         StallReason   reason,
                         cycle_count_t duration) {
  assert(detailedLog);

  if (duration > 0 && component.isCore()) {
    logStream->width(12);
    *logStream << currentCycle;
    logStream->width(12);
    *logStream << component.getGlobalCoreNumber();
    logStream->width(12);
    *logStream << name(reason);
    logStream->width(12);
    *logStream << duration << endl;
  }
}
