/*
 * Stalls.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include <systemc>
#include "Stalls.h"
#include "../../Datatype/ComponentID.h"
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

CounterMap<ComponentID> Stalls::totalStalls;
CounterMap<ComponentID> Stalls::dataStalls;
CounterMap<ComponentID> Stalls::instructionStalls;
CounterMap<ComponentID> Stalls::outputStalls;
CounterMap<ComponentID> Stalls::bypassStalls;
CounterMap<ComponentID> Stalls::idleTimes;

count_t Stalls::numStalled = 0;
cycle_count_t Stalls::endOfExecution = 0;
cycle_count_t Stalls::loggedCycles = 0;
cycle_count_t Stalls::loggingStarted = NOT_LOGGING;
bool Stalls::endExecutionCalled = false;

void Stalls::init() {
  for (uint i=0; i<NUM_TILES; i++) {
    for (uint j=0; j<COMPONENTS_PER_TILE; j++) {
      ComponentID id(i, j);

      stallReason[id]             = IDLE;//NOT_STALLED;

      startedStalling[id]         = 0;//UNSTALLED;
      startedDataStall[id]        = UNSTALLED;
      startedInstructionStall[id] = UNSTALLED;
      startedOutputStall[id]      = UNSTALLED;
      startedBypassStall[id]      = UNSTALLED;
      startedIdle[id]             = 0;//UNSTALLED;
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

      if (startedStalling[id] != UNSTALLED)
        startedStalling[id] = loggingStarted;
      if (startedDataStall[id] != UNSTALLED)
        startedDataStall[id] = loggingStarted;
      if (startedInstructionStall[id] != UNSTALLED)
        startedInstructionStall[id] = loggingStarted;
      if (startedOutputStall[id] != UNSTALLED)
        startedOutputStall[id] = loggingStarted;
      if (startedBypassStall[id] != UNSTALLED)
        startedBypassStall[id] = loggingStarted;
      if (startedIdle[id] != UNSTALLED)
        startedIdle[id] = loggingStarted;
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
}

void Stalls::stall(const ComponentID& id, StallReason reason) {
  Stalls::stall(id, currentCycle(), reason);
}

void Stalls::unstall(const ComponentID& id, StallReason reason) {
  Stalls::unstall(id, currentCycle(), reason);
}

void Stalls::activity(const ComponentID& id, bool idle) {
  if (idle) Stalls::idle(id, currentCycle());
  else Stalls::active(id, currentCycle());
}

void Stalls::stall(const ComponentID& id, cycle_count_t cycle, int reason) {
  // We can't become idle if we're already stalled.
  if ((reason == IDLE) && (stallReason[id] != NOT_STALLED))
    return;

  // If we are stalled, we have work to do, so can't be idle.
  if (stallReason[id] & IDLE)
    unstall(id, cycle, IDLE);

  if (stallReason[id] == NOT_STALLED) {
    numStalled++;
    startedStalling[id] = cycle;

    if (numStalled >= NUM_COMPONENTS)
      endOfExecution = cycle;
  }

  stallReason[id] |= reason;

  switch(reason) {
    case STALL_DATA:         startedDataStall[id] = cycle;        break;
    case STALL_INSTRUCTIONS: startedInstructionStall[id] = cycle; break;
    case STALL_OUTPUT:       startedOutputStall[id] = cycle;      break;
    case STALL_FORWARDING:   startedBypassStall[id] = cycle;      break;
    case IDLE:               startedIdle[id] = cycle;             break;

    default: std::cerr << "Warning: Unknown stall reason." << endl; break;
  }

}

void Stalls::unstall(const ComponentID& id, cycle_count_t cycle, int reason) {
  if (stallReason[id] & reason) {
    switch(reason) {
      case STALL_DATA:
        //if (ENERGY_TRACE)
          dataStalls.setCount(id, dataStalls[id] + cycle - startedDataStall[id]);
        startedDataStall[id] = UNSTALLED;
        break;

      case STALL_INSTRUCTIONS:
        //if (ENERGY_TRACE)
          instructionStalls.setCount(id, instructionStalls[id] + cycle - startedInstructionStall[id]);
        startedInstructionStall[id] = UNSTALLED;
        break;

      case STALL_OUTPUT:
        //if (ENERGY_TRACE)
          outputStalls.setCount(id, outputStalls[id] + cycle - startedOutputStall[id]);
        startedOutputStall[id] = UNSTALLED;
        break;

      case STALL_FORWARDING:
        //if (ENERGY_TRACE)
          bypassStalls.setCount(id, bypassStalls[id] + cycle - startedBypassStall[id]);
        startedBypassStall[id] = UNSTALLED;
        break;

      case IDLE:
        //if (ENERGY_TRACE)
          idleTimes.setCount(id, idleTimes[id] + cycle - startedIdle[id]);
        startedIdle[id] = UNSTALLED;
        break;

      default: std::cerr << "Warning: Unknown stall reason." << endl; break;
    }

    // Clear this stall reason from the bitmask.
    stallReason[id] &= ~reason;

    if (stallReason[id] == NOT_STALLED) {
      numStalled--;
      //if (ENERGY_TRACE)
        totalStalls.setCount(id, totalStalls[id] + cycle - startedStalling[id]);
      startedStalling[id] = UNSTALLED;
    }
  }
}

void Stalls::idle(const ComponentID& id, cycle_count_t cycle) {
  stall(id, cycle, IDLE);
}

void Stalls::active(const ComponentID& id, cycle_count_t cycle) {
  // Only become active if we were idle. If we were properly stalled, waiting
  // for information, then we are still stalled.
//  if (stallReason[id] & IDLE)
    unstall(id, cycle, IDLE);
}

void Stalls::endExecution() {
  if(numStalled < NUM_COMPONENTS)
    endOfExecution = sc_core::sc_time_stamp().to_default_time_units();

  endExecutionCalled = true;
}

bool Stalls::executionFinished() {
  return endExecutionCalled;
}

cycle_count_t Stalls::cyclesActive(const ComponentID& core) {
  if (loggedCycles > 0)
    return loggedCycles - cyclesStalled(core) - cyclesIdle(core);
  else
    return currentCycle() - cyclesStalled(core) - cyclesIdle(core);
}

cycle_count_t Stalls::cyclesIdle(const ComponentID& core) {
  return idleTimes[core];
}

cycle_count_t Stalls::cyclesStalled(const ComponentID& core) {
  return totalStalls[core] - cyclesIdle(core);
}

cycle_count_t Stalls::cyclesLogged() {
  return loggedCycles;
}

cycle_count_t Stalls::executionTime() {
  return endOfExecution;
}

void Stalls::printStats() {
  //TODO: Add information to database if required

  if (endOfExecution == 0) return;

  cout << "Core activity:" << endl;
  cout << "  Core\t\tActive\tIdle\tStalled (insts|data|bypass|output)" << endl;

  // Flush any remaining stall/idle time into the CounterMaps.
  stopLogging();

	for (uint i=0; i<NUM_TILES; i++) {
		for (uint j=0; j<COMPONENTS_PER_TILE; j++) {
			ComponentID id(i, j);

			// Skip over memories for now -- they are not instrumented properly.
			if (id.isMemory()) continue;

			// Only print statistics for cores which have seen some activity.
			if ((uint)idleTimes[id] < endOfExecution) {
				cycle_count_t totalStalled = cyclesStalled(id);
				cycle_count_t activeCycles = cyclesActive(id);
				cout << "  " << id << "\t\t" <<
				percentage(activeCycles, endOfExecution) << "\t" <<
				percentage(idleTimes[id], endOfExecution) << "\t" <<
				percentage(totalStalled, endOfExecution) << "\t(" <<
        percentage(instructionStalls[id], totalStalled) << "|" <<
        percentage(dataStalls[id], totalStalled) << "|" <<
				percentage(bypassStalls[id], totalStalled) << "|" <<
				percentage(outputStalls[id], totalStalled) << ")" << endl;
			}
		}
	}

  if (BATCH_MODE)
	cout << "<@GLOBAL>total_cycles:" << endOfExecution << "</@GLOBAL>" << endl;

  cout << "Total execution time: " << endOfExecution << " cycles" << endl;

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

      cycle_count_t totalStalled = cyclesStalled(id);
      cycle_count_t activeCycles = cyclesActive(id);

      os << "<activity core=\"" << id.getGlobalCoreNumber()  << "\">\n"
         << xmlNode("active", activeCycles)                     << "\n"
         << xmlNode("idle", idleTimes[id])                      << "\n"
         << xmlNode("stalled", totalStalled)                    << "\n"
         << xmlNode("instruction_stall", instructionStalls[id]) << "\n"
         << xmlNode("data_stall", dataStalls[id])               << "\n"
         << xmlNode("bypass_stall", bypassStalls[id])           << "\n"
         << xmlNode("output_stall", outputStalls[id])           << "\n"
         << xmlEnd("activity")                                  << "\n";
    }
  }
}
