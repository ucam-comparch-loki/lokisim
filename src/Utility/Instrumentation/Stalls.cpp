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

std::map<ComponentID, uint> Stalls::stallReason;

std::vector<std::map<ComponentID, cycle_count_t> > Stalls::startStall;

std::vector<CounterMap<ComponentID> > Stalls::total;
std::vector<CounterMap<ComponentID> > Stalls::loggedOnly;

count_t Stalls::numStalled = 0;
cycle_count_t Stalls::endOfExecution = 0;
cycle_count_t Stalls::loggedCycles = 0;
cycle_count_t Stalls::loggingStarted = NOT_LOGGING;
bool Stalls::endExecutionCalled = false;

bool Stalls::detailedLog = false;
std::ofstream* Stalls::logStream;

void Stalls::init() {
  for (uint i=0; i<NUM_STALL_REASONS; i++) {
    total.push_back(CounterMap<ComponentID>());
    loggedOnly.push_back(CounterMap<ComponentID>());
    startStall.push_back(map<ComponentID, cycle_count_t>());
  }

  for (uint i=0; i<NUM_TILES; i++) {
    for (uint j=0; j<COMPONENTS_PER_TILE; j++) {
      ComponentID id(i, j);

      stallReason[id]             = 1 << IDLE;
      numStalled++;

      startStall[STALL_ANY][id]           = 0;
      startStall[STALL_MEMORY_DATA][id]   = UNSTALLED;
      startStall[STALL_CORE_DATA][id]     = UNSTALLED;
      startStall[STALL_INSTRUCTIONS][id]  = UNSTALLED;
      startStall[STALL_OUTPUT][id]        = UNSTALLED;
      startStall[STALL_FORWARDING][id]    = UNSTALLED;
      startStall[STALL_FETCH][id]         = UNSTALLED;
      startStall[IDLE][id]                = 0;
    }
  }
}

void Stalls::startLogging() {
  loggingStarted = currentCycle();

  // If any cores are stalled, pretend that they only just started, so cycles
  // before logging started aren't counted.
  for (uint i=0; i<NUM_TILES; i++) {
    for (uint j=0; j<COMPONENTS_PER_TILE; j++) {
      ComponentID id(i, j);

      for (uint k=0; k<NUM_STALL_REASONS; k++) {
        if (startStall[k][id] != UNSTALLED)
          startStall[k][id] = loggingStarted;
      }
    }
  }
}

void Stalls::stopLogging() {
  if (loggingStarted == NOT_LOGGING)
    return;

  if (loggingStarted != NOT_LOGGING)
    loggedCycles += currentCycle() - loggingStarted;


  // Unstall all cores so their stall durations are added to the totals.
  // No particular instruction caused this, so use a dummy one.
  static Instruction nullInst(0);
  static const DecodedInst decoded(nullInst);

  for (uint i=0; i<NUM_TILES; i++) {
    for (uint j=0; j<COMPONENTS_PER_TILE; j++) {
      ComponentID id(i, j);

      if (startStall[STALL_MEMORY_DATA][id] != UNSTALLED) {
        unstall(id, STALL_MEMORY_DATA, decoded);
        stall(id, STALL_MEMORY_DATA, decoded);
      }
      if (startStall[STALL_CORE_DATA][id] != UNSTALLED) {
        unstall(id, STALL_CORE_DATA, decoded);
        stall(id, STALL_CORE_DATA, decoded);
      }
      if (startStall[STALL_INSTRUCTIONS][id] != UNSTALLED) {
        unstall(id, STALL_INSTRUCTIONS, decoded);
        stall(id, STALL_INSTRUCTIONS, decoded);
      }
      if (startStall[STALL_OUTPUT][id] != UNSTALLED) {
        unstall(id, STALL_OUTPUT, decoded);
        stall(id, STALL_OUTPUT, decoded);
      }
      if (startStall[STALL_FORWARDING][id] != UNSTALLED) {
        unstall(id, STALL_FORWARDING, decoded);
        stall(id, STALL_FORWARDING, decoded);
      }
      if (startStall[STALL_FETCH][id] != UNSTALLED) {
        unstall(id, STALL_FETCH, decoded);
        stall(id, STALL_FETCH, decoded);
      }
      if (startStall[IDLE][id] != UNSTALLED) {
        unstall(id, IDLE, decoded);
        stall(id, IDLE, decoded);
      }
    }
  }

  loggingStarted = NOT_LOGGING;

  if (detailedLog)
    logStream->close();
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
  *logStream << "duration";
  *logStream << " (instruction causing stall)" << endl;
}

void Stalls::stall(const ComponentID id, StallReason reason, const DecodedInst& inst) {
  Stalls::stall(id, currentCycle(), reason, inst);
}

void Stalls::unstall(const ComponentID id, StallReason reason, const DecodedInst& inst) {
  Stalls::unstall(id, currentCycle(), reason, inst);
}

void Stalls::activity(const ComponentID id, bool idle) {
  if (idle) Stalls::idle(id, currentCycle());
  else Stalls::active(id, currentCycle());
}

void Stalls::stall(const ComponentID id, cycle_count_t cycle, StallReason reason, const DecodedInst& inst) {
  uint bitmask = 1 << reason;

  // We're already stalled for this reason.
  if (stallReason[id] & bitmask)
    return;

  // We can't become idle if we're already stalled.
  if ((reason == IDLE) && (stallReason[id] != NOT_STALLED))
    return;

  // If we are stalled, we have work to do, so can't be idle.
  if (stallReason[id] & IDLE)
    unstall(id, cycle, IDLE, inst);

  if (stallReason[id] == NOT_STALLED) {
    numStalled++;
    assert(numStalled <= NUM_COMPONENTS);

    startStall[STALL_ANY][id] = cycle;

    if (numStalled >= NUM_COMPONENTS)
      endOfExecution = cycle;
  }

  stallReason[id] |= bitmask;
  startStall[reason][id] = cycle;

}

cycle_count_t max(cycle_count_t time1, cycle_count_t time2) {
  return (time1 > time2) ? time1 : time2;
}

void Stalls::unstall(const ComponentID id, cycle_count_t cycle, StallReason reason, const DecodedInst& inst) {
  uint bitmask = 1 << reason;

  if (stallReason[id] & bitmask) {
    assert(startStall[reason][id] != UNSTALLED);
    assert(startStall[STALL_ANY][id] != UNSTALLED);

    cycle_count_t timeStalled = 0;

    switch (reason) {
      case NOT_STALLED:
      case STALL_ANY:
        // Special cases - do nothing.
        break;

      default:
        if (ENERGY_TRACE)
          loggedOnly[reason].increment(id, cycle - max(loggingStarted, startStall[reason][id]));
        timeStalled = cycle - startStall[reason][id];
        total[reason].increment(id, timeStalled);
        startStall[reason][id] = UNSTALLED;
        break;
    }

    // Clear this stall reason from the bitmask.
    stallReason[id] &= ~bitmask;

    if (detailedLog)
      recordEvent(cycle, id, reason, timeStalled, inst);

    if (stallReason[id] == NOT_STALLED) {
      numStalled--;
      assert(numStalled <= NUM_COMPONENTS);

      if (ENERGY_TRACE)
        loggedOnly[STALL_ANY].increment(id, cycle - max(loggingStarted, startStall[STALL_ANY][id]));
      total[STALL_ANY].increment(id, cycle - startStall[STALL_ANY][id]);
      startStall[STALL_ANY][id] = UNSTALLED;
    }
  }
}

void Stalls::idle(const ComponentID id, cycle_count_t cycle) {
  // Idleness isn't caused by any particular instruction, so provide a dummy.
  static Instruction nullInst(0);
  static const DecodedInst decoded(nullInst);
  stall(id, cycle, IDLE, decoded);
}

void Stalls::active(const ComponentID id, cycle_count_t cycle) {
  // Idleness isn't caused by any particular instruction, so provide a dummy.
  static Instruction nullInst(0);
  static const DecodedInst decoded(nullInst);
  unstall(id, cycle, IDLE, decoded);
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
  return total[IDLE][core];
}

cycle_count_t Stalls::cyclesStalled(const ComponentID core) {
  return total[STALL_ANY][core] - cyclesIdle(core);
}

cycle_count_t Stalls::cyclesLogged() {
  return loggedCycles;
}

cycle_count_t Stalls::loggedCyclesActive(const ComponentID core) {
  return cyclesLogged() - loggedCyclesStalled(core) - loggedCyclesIdle(core);
}

cycle_count_t Stalls::loggedCyclesIdle(const ComponentID core) {
  return loggedOnly[IDLE][core];
}

cycle_count_t Stalls::loggedCyclesStalled(const ComponentID core) {
  return loggedOnly[STALL_ANY][core] - loggedCyclesIdle(core);
}

cycle_count_t Stalls::executionTime() {
  return endOfExecution;
}

void Stalls::printInstrStat(const char *name, ComponentID id, CounterMap<ComponentID> &cMap) {
  std::clog << name << ": " << cMap[id] << " (" << percentage(cMap[id], Operations::numOperations(id)) << ")\n";
}


void Stalls::printStats() {

  using std::clog;

  if (endOfExecution == 0) return;

  clog << "Total execution time: " << endOfExecution << " cycles" << endl;

  clog << "Core activity:" << endl;
  clog << "  Core\tInsts\tActive\tIdle\tStalled (inst|mem|core|fwd|fetch|out)" << endl;

  // Flush any remaining stall/idle time into the CounterMaps.
  stopLogging();

	for (uint i=0; i<NUM_TILES; i++) {
		for (uint j=0; j<COMPONENTS_PER_TILE; j++) {
			ComponentID id(i, j);

			// Skip over memories for now -- they are not instrumented properly.
			if (id.isMemory()) continue;

			// Only print statistics for cores which have seen some activity.
			if ((uint)total[IDLE][id] < endOfExecution) {
				cycle_count_t totalStalled = cyclesStalled(id);
				cycle_count_t activeCycles = cyclesActive(id);
				clog << "  " << id << "\t" <<
				Operations::numOperations(id) << "\t" <<
				percentage(activeCycles, endOfExecution) << "\t" <<
				percentage(total[IDLE][id], endOfExecution) << "\t" <<
				percentage(totalStalled, endOfExecution) << "\t(" <<
        percentage(total[STALL_INSTRUCTIONS][id], totalStalled) << "|" <<
        percentage(total[STALL_MEMORY_DATA][id], totalStalled) << "|" <<
        percentage(total[STALL_CORE_DATA][id], totalStalled) << "|" <<
        percentage(total[STALL_FORWARDING][id], totalStalled) << "|" <<
        percentage(total[STALL_FETCH][id], totalStalled) << "|" <<
				percentage(total[STALL_OUTPUT][id], totalStalled) << ")" << endl;
			}
		}
	}

  // Print instruction distribution summary
  clog << "\nDistribution of instructions:\n";
  for (uint i=0; i<NUM_TILES; i++) {
    for (uint j=0; j<COMPONENTS_PER_TILE; j++) {
      ComponentID id(i, j);

      // Skip over memories
      if (id.isMemory()) continue;

      // Only print statistics for cores which have seen some activity.
      if ((uint)total[IDLE][id] < endOfExecution) {
        clog << "  " << id << "\n" <<
          "Total: " << Operations::numOperations(id) << " (100%)\n";
        printInstrStat("numMemLoads", id, Operations::numMemLoads);
        printInstrStat("numMergedMemLoads", id, Operations::numMergedMemLoads);
        printInstrStat("numMemStores", id, Operations::numMemStores);
        printInstrStat("numChanReads", id, Operations::numChanReads);
        printInstrStat("numMergedChanReads", id, Operations::numMergedChanReads);
        printInstrStat("numChanWrites", id, Operations::numChanWrites);
        printInstrStat("numMergedChanWrites", id, Operations::numMergedChanWrites);
        printInstrStat("numArithOps", id, Operations::numArithOps);
        printInstrStat("numCondOps", id, Operations::numCondOps);
        clog << "\n";
      }
    }
  }


  if (BATCH_MODE)
	cout << "<@GLOBAL>total_cycles:" << endOfExecution << "</@GLOBAL>" << endl;

}

void Stalls::dumpEventCounts(std::ostream& os) {
  stopLogging();

  for (uint i=0; i<NUM_TILES; i++) {
    for (uint j=0; j<COMPONENTS_PER_TILE; j++) {
      ComponentID id(i, j);

      // Only interested in cores.
      if (id.isMemory())
        continue;

      // Skip over any cores which were idle the whole time.
//      if ((cycle_count_t)idleTimes[id] >= endOfExecution)
//        continue;

      cycle_count_t totalStalled = loggedCyclesStalled(id);
      cycle_count_t activeCycles = loggedCyclesActive(id);

      assert(totalStalled <= loggedCycles);
      assert(activeCycles <= loggedCycles);

      os << "<activity core=\"" << id.getGlobalCoreNumber()  << "\">\n"
         << xmlNode("active", activeCycles)                     << "\n"
         << xmlNode("idle", loggedOnly[IDLE][id])               << "\n"
         << xmlNode("stalled", totalStalled)                    << "\n"
         << xmlNode("instruction_stall", loggedOnly[STALL_INSTRUCTIONS][id]) << "\n"
         << xmlNode("memory_data_stall", loggedOnly[STALL_MEMORY_DATA][id])  << "\n"
         << xmlNode("core_data_stall", loggedOnly[STALL_CORE_DATA][id])      << "\n"
         << xmlNode("bypass_stall", loggedOnly[STALL_FORWARDING][id])        << "\n"
         << xmlNode("fetch_stall", loggedOnly[STALL_FETCH][id])              << "\n"
         << xmlNode("output_stall", loggedOnly[STALL_OUTPUT][id])            << "\n"
         << xmlEnd("activity")                                  << "\n";
    }
  }
}

const string Stalls::name(StallReason reason) {
  switch (reason) {
    case NOT_STALLED:         return "not stalled";
    case STALL_MEMORY_DATA:   return "memory data";
    case STALL_CORE_DATA:     return "core data";
    case STALL_INSTRUCTIONS:  return "insts";
    case STALL_OUTPUT:        return "output";
    case STALL_FORWARDING:    return "forwarding";
    case STALL_FETCH:         return "fetch";
    case IDLE:                return "idle";
    default: throw InvalidOptionException("stall reason", reason);
  }
}

void Stalls::recordEvent(cycle_count_t currentCycle,
                         ComponentID   component,
                         StallReason   reason,
                         cycle_count_t duration,
                         const DecodedInst& inst) {
  assert(detailedLog);

  if (duration > 0 && component.isCore()) {
    logStream->width(12);
    *logStream << currentCycle;
    logStream->width(12);
    *logStream << component.getGlobalCoreNumber();
    logStream->width(12);
    *logStream << name(reason);
    logStream->width(12);
    *logStream << duration;
    if (reason != IDLE)
      *logStream << " (0x" << std::hex << inst.location() << std::dec << ": " << inst << ")";
    *logStream << endl;
  }
}
