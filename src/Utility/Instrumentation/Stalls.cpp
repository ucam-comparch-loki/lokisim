/*
 * Stalls.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include <systemc>
#include "Stalls.h"
#include "Operations.h"
#include "../../Exceptions/InvalidOptionException.h"
#include "../Instrumentation.h"
#include "../Parameters.h"

using namespace Instrumentation;

// If a core is not currently stalled, it should map to this value in the
// "stalled" mapping.
const cycle_count_t UNSTALLED = -1;

const cycle_count_t NOT_LOGGING = -1;

std::map<ComponentIndex, uint> Stalls::stallReason;

std::vector<std::map<ComponentIndex, cycle_count_t> > Stalls::startStall;

std::vector<CounterMap<ComponentIndex> > Stalls::total;
std::vector<CounterMap<ComponentIndex> > Stalls::loggedOnly;

count_t Stalls::numStalled = 0;
cycle_count_t Stalls::endOfExecution = 0;
cycle_count_t Stalls::loggedCycles = 0;
cycle_count_t Stalls::loggingStarted = NOT_LOGGING;
bool Stalls::endExecutionCalled = false;

bool Stalls::detailedLog = false;
std::ofstream* Stalls::logStream;

void Stalls::init() {
  total.clear();
  loggedOnly.clear();
  startStall.clear();

  numStalled = 0;
  endOfExecution = 0;
  loggedCycles = 0;

  for (uint i=0; i<NUM_STALL_REASONS; i++) {
    total.push_back(CounterMap<ComponentIndex>());
    loggedOnly.push_back(CounterMap<ComponentIndex>());
    startStall.push_back(map<ComponentIndex, cycle_count_t>());
  }

  for (uint col = 1; col <= COMPUTE_TILE_COLUMNS; col++) {
    for (uint row = 1; row <= COMPUTE_TILE_ROWS; row++) {
      for (uint component=0; component<COMPONENTS_PER_TILE; component++) {
        ComponentID id(col, row, component);
        ComponentIndex index = id.globalComponentNumber();

        stallReason[index]                     = 1 << IDLE;
        numStalled++;

        startStall[STALL_ANY][index]           = 0;
        startStall[STALL_MEMORY_DATA][index]   = UNSTALLED;
        startStall[STALL_CORE_DATA][index]     = UNSTALLED;
        startStall[STALL_INSTRUCTIONS][index]  = UNSTALLED;
        startStall[STALL_OUTPUT][index]        = UNSTALLED;
        startStall[STALL_FORWARDING][index]    = UNSTALLED;
        startStall[STALL_FETCH][index]         = UNSTALLED;
        startStall[IDLE][index]                = 0;
      }
    }
  }
}

void Stalls::startLogging() {
  loggingStarted = currentCycle();

  // If any cores are stalled, pretend that they only just started, so cycles
  // before logging started aren't counted.
  for (uint col = 1; col <= COMPUTE_TILE_COLUMNS; col++) {
    for (uint row = 1; row <= COMPUTE_TILE_ROWS; row++) {
      for (uint component=0; component<COMPONENTS_PER_TILE; component++) {
        ComponentID id(col, row, component);
        ComponentIndex index = id.globalComponentNumber();

        for (uint reason=0; reason<NUM_STALL_REASONS; reason++) {
          if (startStall[reason][index] != UNSTALLED)
            startStall[reason][index] = loggingStarted;
        }
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

  for (uint col = 1; col <= COMPUTE_TILE_COLUMNS; col++) {
    for (uint row = 1; row <= COMPUTE_TILE_ROWS; row++) {
      for (uint component=0; component<COMPONENTS_PER_TILE; component++) {
        ComponentID id(col, row, component);
        ComponentIndex index = id.globalComponentNumber();

        if (startStall[STALL_MEMORY_DATA][index] != UNSTALLED) {
          unstall(id, STALL_MEMORY_DATA, decoded);
          stall(id, STALL_MEMORY_DATA, decoded);
        }
        if (startStall[STALL_CORE_DATA][index] != UNSTALLED) {
          unstall(id, STALL_CORE_DATA, decoded);
          stall(id, STALL_CORE_DATA, decoded);
        }
        if (startStall[STALL_INSTRUCTIONS][index] != UNSTALLED) {
          unstall(id, STALL_INSTRUCTIONS, decoded);
          stall(id, STALL_INSTRUCTIONS, decoded);
        }
        if (startStall[STALL_OUTPUT][index] != UNSTALLED) {
          unstall(id, STALL_OUTPUT, decoded);
          stall(id, STALL_OUTPUT, decoded);
        }
        if (startStall[STALL_FORWARDING][index] != UNSTALLED) {
          unstall(id, STALL_FORWARDING, decoded);
          stall(id, STALL_FORWARDING, decoded);
        }
        if (startStall[STALL_FETCH][index] != UNSTALLED) {
          unstall(id, STALL_FETCH, decoded);
          stall(id, STALL_FETCH, decoded);
        }
        if (startStall[IDLE][index] != UNSTALLED) {
          unstall(id, IDLE, decoded);
          stall(id, IDLE, decoded);
        }
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
  ComponentIndex index = id.globalComponentNumber();

  // We're already stalled for this reason.
  if (stallReason[index] & bitmask)
    return;

  // We can't become idle if we're already stalled.
  if ((reason == IDLE) && (stallReason[index] != NOT_STALLED))
    return;

  // If we are stalled, we have work to do, so can't be idle.
  if (stallReason[index] & IDLE)
    unstall(id, cycle, IDLE, inst);

  if (stallReason[index] == NOT_STALLED) {
    numStalled++;
    assert(numStalled <= NUM_COMPONENTS);

    startStall[STALL_ANY][index] = cycle;

    if (numStalled >= NUM_COMPONENTS)
      endOfExecution = cycle;
  }

  stallReason[index] |= bitmask;
  startStall[reason][index] = cycle;

}

cycle_count_t max(cycle_count_t time1, cycle_count_t time2) {
  return (time1 > time2) ? time1 : time2;
}

void Stalls::unstall(const ComponentID id, cycle_count_t cycle, StallReason reason, const DecodedInst& inst) {
  uint bitmask = 1 << reason;
  ComponentIndex index = id.globalComponentNumber();

  if (stallReason[index] & bitmask) {
    assert(startStall[reason][index] != UNSTALLED);
    assert(startStall[STALL_ANY][index] != UNSTALLED);

    cycle_count_t timeStalled = 0;

    switch (reason) {
      case NOT_STALLED:
      case STALL_ANY:
        // Special cases - do nothing.
        break;

      default:
        if (ENERGY_TRACE)
          loggedOnly[reason].increment(index, cycle - max(loggingStarted, startStall[reason][index]));
        timeStalled = cycle - startStall[reason][index];
        total[reason].increment(index, timeStalled);
        startStall[reason][index] = UNSTALLED;
        break;
    }

    // Clear this stall reason from the bitmask.
    stallReason[index] &= ~bitmask;

    if (detailedLog)
      recordEvent(cycle, index, reason, timeStalled, inst);

    if (stallReason[index] == NOT_STALLED) {
      numStalled--;
      assert(numStalled <= NUM_COMPONENTS);

      if (ENERGY_TRACE)
        loggedOnly[STALL_ANY].increment(index, cycle - max(loggingStarted, startStall[STALL_ANY][index]));
      total[STALL_ANY].increment(index, cycle - startStall[STALL_ANY][index]);
      startStall[STALL_ANY][index] = UNSTALLED;
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
    endOfExecution = Instrumentation::currentCycle();

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
  return total[IDLE][core.globalComponentNumber()];
}

cycle_count_t Stalls::cyclesStalled(const ComponentID core) {
  return total[STALL_ANY][core.globalComponentNumber()] - cyclesIdle(core);
}

cycle_count_t Stalls::cyclesLogged() {
  return loggedCycles;
}

cycle_count_t Stalls::loggedCyclesActive(const ComponentID core) {
  return cyclesLogged() - loggedCyclesStalled(core) - loggedCyclesIdle(core);
}

cycle_count_t Stalls::loggedCyclesIdle(const ComponentID core) {
  return loggedOnly[IDLE][core.globalComponentNumber()];
}

cycle_count_t Stalls::loggedCyclesStalled(const ComponentID core) {
  return loggedOnly[STALL_ANY][core.globalComponentNumber()] - loggedCyclesIdle(core);
}

cycle_count_t Stalls::executionTime() {
  return endOfExecution;
}

void Stalls::printInstrStat(const char *name, ComponentID id, CounterMap<CoreIndex> &cMap) {
  std::clog << name << ": " << cMap[id.globalCoreNumber()] << " (" << percentage(cMap[id.globalCoreNumber()], Operations::numOperations(id)) << ")\n";
}


void Stalls::printStats() {

  using std::clog;

  if (endOfExecution == 0) return;

  clog << "Total execution time: " << endOfExecution << " cycles" << endl;

  clog << "Core activity:" << endl;
  clog << "  Core\tInsts\tActive\tIdle\tStalled (inst|mem|core|fwd|fetch|out)" << endl;

  // Flush any remaining stall/idle time into the CounterMaps.
  stopLogging();

  for (uint col = 1; col <= COMPUTE_TILE_COLUMNS; col++) {
    for (uint row = 1; row <= COMPUTE_TILE_ROWS; row++) {
      for (uint component=0; component<COMPONENTS_PER_TILE; component++) {
        ComponentID id(col, row, component);
        ComponentIndex index = id.globalComponentNumber();

        // Skip over memories for now -- they are not instrumented properly.
        if (id.isMemory()) continue;

        // Only print statistics for cores which have seen some activity.
        if ((uint)total[IDLE][index] < endOfExecution) {
          cycle_count_t totalStalled = cyclesStalled(id);
          cycle_count_t activeCycles = cyclesActive(id);
          clog << "  " << id << "\t" <<
          Operations::numOperations(id) << "\t" <<
          percentage(activeCycles, endOfExecution) << "\t" <<
          percentage(total[IDLE][index], endOfExecution) << "\t" <<
          percentage(totalStalled, endOfExecution) << "\t(" <<
          percentage(total[STALL_INSTRUCTIONS][index], totalStalled) << "|" <<
          percentage(total[STALL_MEMORY_DATA][index], totalStalled) << "|" <<
          percentage(total[STALL_CORE_DATA][index], totalStalled) << "|" <<
          percentage(total[STALL_FORWARDING][index], totalStalled) << "|" <<
          percentage(total[STALL_FETCH][index], totalStalled) << "|" <<
          percentage(total[STALL_OUTPUT][index], totalStalled) << ")" << endl;
        }
      }
		}
	}

  // Print instruction distribution summary
  clog << "\nDistribution of instructions:\n";

  for (uint col = 1; col <= COMPUTE_TILE_COLUMNS; col++) {
    for (uint row = 1; row <= COMPUTE_TILE_ROWS; row++) {
      for (uint component=0; component<COMPONENTS_PER_TILE; component++) {
        ComponentID id(col, row, component);
  
        // Skip over memories
        if (id.isMemory()) continue;
  
        // Only print statistics for cores which have seen some activity.
        if ((uint)total[IDLE][id.globalComponentNumber()] < endOfExecution) {
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
  }

  if (Arguments::batchMode())
	cout << "<@GLOBAL>total_cycles:" << endOfExecution << "</@GLOBAL>" << endl;

}

void Stalls::dumpEventCounts(std::ostream& os) {
  stopLogging();

  for (uint col = 1; col <= COMPUTE_TILE_COLUMNS; col++) {
    for (uint row = 1; row <= COMPUTE_TILE_ROWS; row++) {
      for (uint component=0; component<COMPONENTS_PER_TILE; component++) {
        ComponentIndex id = ComponentID(col, row, component).globalCoreNumber();

        // Skip over any cores which were idle the whole time.
  //      if ((cycle_count_t)idleTimes[id] >= endOfExecution)
  //        continue;

        cycle_count_t totalStalled = loggedOnly[STALL_ANY][id] - loggedOnly[IDLE][id];
        cycle_count_t activeCycles = loggedCycles - loggedOnly[STALL_ANY][id];

        os << "<activity core=\"" << id  << "\">\n"
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
    *logStream << component.globalCoreNumber();
    logStream->width(12);
    *logStream << name(reason);
    logStream->width(12);
    *logStream << duration;
    if (reason != IDLE)
      *logStream << " (0x" << std::hex << inst.location() << std::dec << ": " << inst << ")";
    *logStream << endl;
  }
}
