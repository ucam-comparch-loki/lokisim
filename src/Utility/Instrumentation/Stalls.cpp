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

std::map<ComponentID, uint> Stalls::stallReason;

std::vector<std::map<ComponentID, cycle_count_t> > Stalls::startStall;

std::vector<CounterMap<ComponentID> > Stalls::timeSpent;

count_t Stalls::numStalled = 0;
cycle_count_t Stalls::endOfExecution = 0;
bool Stalls::endExecutionCalled = false;

bool Stalls::detailedLog = false;
std::ofstream* Stalls::logStream;

void Stalls::init() {
  startStall.clear();

  cycle_count_t now = currentCycle();

  numStalled = 0;
  endOfExecution = 0;

  for (uint i=0; i<NUM_STALL_REASONS; i++) {
    timeSpent.push_back(CounterMap<ComponentID>());
    startStall.push_back(map<ComponentID, cycle_count_t>());
  }

  for (uint col = 1; col <= COMPUTE_TILE_COLUMNS; col++) {
    for (uint row = 1; row <= COMPUTE_TILE_ROWS; row++) {
      for (uint component=0; component<COMPONENTS_PER_TILE; component++) {
        ComponentID id(col, row, component);

        stallReason[id]                     = 1 << IDLE;
        numStalled++;

        startStall[STALL_ANY][id]           = now;
        startStall[STALL_MEMORY_DATA][id]   = UNSTALLED;
        startStall[STALL_CORE_DATA][id]     = UNSTALLED;
        startStall[STALL_INSTRUCTIONS][id]  = UNSTALLED;
        startStall[STALL_OUTPUT][id]        = UNSTALLED;
        startStall[STALL_FORWARDING][id]    = UNSTALLED;
        startStall[STALL_FETCH][id]         = UNSTALLED;
        startStall[IDLE][id]                = now;
      }
    }
  }
}

void Stalls::reset() {
  cycle_count_t now = currentCycle();

  // If any cores are stalled, pretend that they only just started, so cycles
  // before reset aren't counted.
  for (uint col = 1; col <= COMPUTE_TILE_COLUMNS; col++) {
    for (uint row = 1; row <= COMPUTE_TILE_ROWS; row++) {
      for (uint component=0; component<COMPONENTS_PER_TILE; component++) {
        ComponentID id(col, row, component);

        for (uint reason=0; reason<NUM_STALL_REASONS; reason++) {
          if (startStall[reason][id] != UNSTALLED) {
            startStall[reason][id] = now;
          }
        }
      }
    }
  }

  for (uint i=0; i<timeSpent.size(); i++)
    timeSpent[i].clear();
}

void Stalls::start() {
  if (collectingStats())
    return;

  cycle_count_t now = currentCycle();

  // If any cores are stalled, pretend that they only just started, so cycles
  // before logging started aren't counted.
  for (uint col = 1; col <= COMPUTE_TILE_COLUMNS; col++) {
    for (uint row = 1; row <= COMPUTE_TILE_ROWS; row++) {
      for (uint component=0; component<COMPONENTS_PER_TILE; component++) {
        ComponentID id(col, row, component);

        for (uint reason=0; reason<NUM_STALL_REASONS; reason++) {
          if (startStall[reason][id] != UNSTALLED) {
            startStall[reason][id] = now;
          }
        }
      }
    }
  }
}

void Stalls::stop() {
  if (!collectingStats())
    return;

  // Unstall all cores so their stall durations are added to the totals.
  // No particular instruction caused this, so use a dummy one.
  static Instruction nullInst(0);
  static const DecodedInst decoded(nullInst);

  for (uint col = 1; col <= COMPUTE_TILE_COLUMNS; col++) {
    for (uint row = 1; row <= COMPUTE_TILE_ROWS; row++) {
      for (uint component=0; component<COMPONENTS_PER_TILE; component++) {
        ComponentID id(col, row, component);

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
  }

}

void Stalls::end() {
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
        if (collectingStats())
          timeSpent[reason].increment(id, cycle - startStall[reason][id]);
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

      if (collectingStats())
        timeSpent[STALL_ANY].increment(id, cycle - startStall[STALL_ANY][id]);
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
  return cyclesStatsCollected() - cyclesStalled(core) - cyclesIdle(core);
}

cycle_count_t Stalls::cyclesIdle(const ComponentID core) {
  return timeSpent[IDLE][core];
}

cycle_count_t Stalls::cyclesStalled(const ComponentID core) {
  return timeSpent[STALL_ANY][core] - cyclesIdle(core);
}

cycle_count_t Stalls::executionTime() {
  return endOfExecution;
}

void Stalls::printInstrStat(const char *name, ComponentID id, CounterMap<ComponentID> &cMap) {
  std::clog << name << ": " << cMap[id] << " (" << percentage(cMap[id], Operations::numOperations(id)) << ")\n";
}


void Stalls::printStats() {

  using std::clog;

  cycle_count_t totalCycles = cyclesStatsCollected();

  if (totalCycles == 0) return;

  clog << "Statistics recorded for " << totalCycles << " cycles" << endl;

  clog << "Core activity:" << endl;
  clog << "  Core\t\tInsts\tActive\tIdle\tStalled (inst|mem|core|fwd|fetch|out)" << endl;

  // Flush any remaining stall/idle time into the CounterMaps.
  stop();

  for (uint col = 1; col <= COMPUTE_TILE_COLUMNS; col++) {
    for (uint row = 1; row <= COMPUTE_TILE_ROWS; row++) {
      for (uint component=0; component<COMPONENTS_PER_TILE; component++) {
        ComponentID id(col, row, component);

        // Skip over memories for now -- they are not instrumented properly.
        if (id.isMemory()) continue;

        // Only print statistics for cores which have seen some activity.
        if (Operations::numOperations(id) > 0) {
          cycle_count_t totalStalled = cyclesStalled(id);
          cycle_count_t activeCycles = cyclesActive(id);
          clog << "  " << id << "\t" <<
          Operations::numOperations(id) << "\t" <<
          percentage(activeCycles, totalCycles) << "\t" <<
          percentage(timeSpent[IDLE][id], totalCycles) << "\t" <<
          percentage(totalStalled, totalCycles) << "\t(" <<
          percentage(timeSpent[STALL_INSTRUCTIONS][id], totalStalled) << "|" <<
          percentage(timeSpent[STALL_MEMORY_DATA][id], totalStalled) << "|" <<
          percentage(timeSpent[STALL_CORE_DATA][id], totalStalled) << "|" <<
          percentage(timeSpent[STALL_FORWARDING][id], totalStalled) << "|" <<
          percentage(timeSpent[STALL_FETCH][id], totalStalled) << "|" <<
          percentage(timeSpent[STALL_OUTPUT][id], totalStalled) << ")" << endl;
        }
      }
    }
  }

  // Everything below this point is very rarely wanted.
  return;

  // Print instruction distribution summary
  clog << "\nDistribution of instructions:\n";

  for (uint col = 1; col <= COMPUTE_TILE_COLUMNS; col++) {
    for (uint row = 1; row <= COMPUTE_TILE_ROWS; row++) {
      for (uint component=0; component<COMPONENTS_PER_TILE; component++) {
        ComponentID id(col, row, component);
  
        // Skip over memories
        if (id.isMemory()) continue;
  
        // Only print statistics for cores which have seen some activity.
        if (Operations::numOperations(id) > 0) {
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

}

void Stalls::dumpEventCounts(std::ostream& os) {
  stop();

  for (uint col = 1; col <= COMPUTE_TILE_COLUMNS; col++) {
    for (uint row = 1; row <= COMPUTE_TILE_ROWS; row++) {
      for (uint component=0; component<COMPONENTS_PER_TILE; component++) {
        ComponentID id(col, row, component);

        // Skip over any cores which were idle the whole time.
  //      if ((cycle_count_t)idleTimes[id] >= endOfExecution)
  //        continue;

        cycle_count_t totalStalled = timeSpent[STALL_ANY][id] - timeSpent[IDLE][id];
        cycle_count_t activeCycles = cyclesStatsCollected() - timeSpent[STALL_ANY][id];

        os << "<activity core=\"" << id  << "\">\n"
           << xmlNode("active", activeCycles)                     << "\n"
           << xmlNode("idle", timeSpent[IDLE][id])               << "\n"
           << xmlNode("stalled", totalStalled)                    << "\n"
           << xmlNode("instruction_stall", timeSpent[STALL_INSTRUCTIONS][id]) << "\n"
           << xmlNode("memory_data_stall", timeSpent[STALL_MEMORY_DATA][id])  << "\n"
           << xmlNode("core_data_stall", timeSpent[STALL_CORE_DATA][id])      << "\n"
           << xmlNode("bypass_stall", timeSpent[STALL_FORWARDING][id])        << "\n"
           << xmlNode("fetch_stall", timeSpent[STALL_FETCH][id])              << "\n"
           << xmlNode("output_stall", timeSpent[STALL_OUTPUT][id])            << "\n"
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
