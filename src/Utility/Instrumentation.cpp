/*
 * Instrumentation.cpp
 *
 *  Created on: 16 Jun 2010
 *      Author: db434
 */

#include <iomanip>
#include "Instrumentation.h"
#include "StartUp/CodeLoader.h"
#include "Arguments.h"
#include "Debugger.h"
#include "Instrumentation/ChannelMap.h"
#include "Instrumentation/FIFO.h"
#include "Instrumentation/IPKCache.h"
#include "Instrumentation/Latency.h"
#include "Instrumentation/MainMemory.h"
#include "Instrumentation/MemoryBank.h"
#include "Instrumentation/Network.h"
#include "Instrumentation/Operations.h"
#include "Instrumentation/PipelineReg.h"
#include "Instrumentation/Registers.h"
#include "Instrumentation/Scratchpad.h"
#include "Instrumentation/Stalls.h"
#include "../Datatype/DecodedInst.h"

// The time at which the statistics were last reset.
cycle_count_t statsWiped = 0;
cycle_count_t statsStarted = 0;
cycle_count_t statsStopped = 0;

// Are we currently collecting statistics?
bool collecting = false;
cycle_count_t cyclesRecorded = 0;

void Instrumentation::initialise() {
  ChannelMap::init();
  FIFO::init();
  IPKCache::init();
  Latency::init();
  MainMemory::init();
  MemoryBank::init();
  Network::init();
  Operations::init();
  PipelineReg::init();
  Registers::init();
  Scratchpad::init();
  Stalls::init();

  reset();
}

void Instrumentation::reset() {
  ChannelMap::reset();
  FIFO::reset();
  IPKCache::reset();
  Latency::reset();
  MainMemory::reset();
  MemoryBank::reset();
  Network::reset();
  Operations::reset();
  PipelineReg::reset();
  Registers::reset();
  Scratchpad::reset();
  Stalls::reset();

  statsWiped = currentCycle();
  if (collecting)
    statsStarted = currentCycle();
  cyclesRecorded = 0;
}

void Instrumentation::start() {
  ChannelMap::start();
  FIFO::start();
  IPKCache::start();
  Latency::start();
  MainMemory::start();
  MemoryBank::start();
  Network::start();
  Operations::start();
  PipelineReg::start();
  Registers::start();
  Scratchpad::start();
  Stalls::start();

  if (!collecting)
    statsStarted = currentCycle();
  collecting = true;
}

void Instrumentation::stop() {
  ChannelMap::stop();
  FIFO::stop();
  IPKCache::stop();
  Latency::stop();
  MainMemory::stop();
  MemoryBank::stop();
  Network::stop();
  Operations::stop();
  PipelineReg::stop();
  Registers::stop();
  Scratchpad::stop();
  Stalls::stop();

  if (collecting) {
    statsStopped = currentCycle();
    cyclesRecorded += statsStopped - statsStarted;
  }

  collecting = false;
}

void Instrumentation::end() {
  stop();

  ChannelMap::end();
  FIFO::end();
  IPKCache::end();
  Latency::end();
  MainMemory::end();
  MemoryBank::end();
  Network::end();
  Operations::end();
  PipelineReg::end();
  Registers::end();
  Scratchpad::end();
  Stalls::end();
}

bool Instrumentation::collectingStats() {
  return collecting;
}

cycle_count_t Instrumentation::cyclesStatsCollected() {
  return cyclesRecorded;
}

cycle_count_t Instrumentation::startedCollectingStats() {
  return statsStarted;
}

cycle_count_t Instrumentation::stoppedCollectingStats() {
  return statsStopped;
}

void Instrumentation::dumpEventCounts(std::ostream& os) {
  os << "<lokitrace>\n\n";

  os << "<invocation>" << Arguments::invocation() << "</invocation>\n";
  os << "\n";
  os << "<time>" << time(NULL) << "</time>\n";
  os << "\n";
  Parameters::printParametersXML(os);
  os << "\n";
  os << "<cycles>" << cyclesStatsCollected() << "</cycles>\n";
  os << "\n";

  ChannelMap::dumpEventCounts(os);    os << "\n";
  FIFO::dumpEventCounts(os);          os << "\n";
  IPKCache::dumpEventCounts(os);      os << "\n";
  MemoryBank::dumpEventCounts(os);    os << "\n";
  Network::dumpEventCounts(os);       os << "\n";
  Operations::dumpEventCounts(os);    os << "\n";
  PipelineReg::dumpEventCounts(os);   os << "\n";
  Registers::dumpEventCounts(os);     os << "\n";
  Scratchpad::dumpEventCounts(os);    os << "\n";
  Stalls::dumpEventCounts(os);        os << "\n";

  os << "</lokitrace>\n";
}

void Instrumentation::printSummary() {
  Stalls::printStats();
  IPKCache::printSummary();
  MemoryBank::printSummary();
  MainMemory::printStats();
  Latency::printSummary();
  Network::printSummary();
  Operations::printSummary();
}

bool Instrumentation::haveEnergyData() {
  // Assume that if we have collected energy data, at least one instruction
  // has been executed.
  return ENERGY_TRACE && cyclesStatsCollected() > 0;
}

void Instrumentation::decoded(const ComponentID& core, const DecodedInst& dec) {
  Operations::decoded(core, dec);

  if (Arguments::instructionAddressTrace()) {
    MemoryAddr location = dec.location();
    std::cout << "0x";
    std::cout.width(8);
    std::cout << std::hex << std::setfill('0') << location << endl;
  }

  if (Arguments::instructionTrace()) {
    std::cout << dec << endl;
  }
}


void Instrumentation::idle(const ComponentID id, bool idle) {
  if (idle) Stalls::idle(id, currentCycle());
  else Stalls::active(id, currentCycle());
}

void Instrumentation::endExecution() {
  Stalls::endExecution();
  stop();

  // Only end simulation if we aren't using the debugger: we may still want to
  // probe memory contents.
  if (!Debugger::usingDebugger) sc_stop();
}

bool Instrumentation::executionFinished() {
  return Stalls::executionFinished();
}

void Instrumentation::executed(const ComponentID& id, const DecodedInst& inst, bool executed) {
  Operations::executed(id, inst, executed);

  if (Debugger::mode == Debugger::DEBUGGER)
    Debugger::executedInstruction(inst, id, executed);
}

cycle_count_t Instrumentation::currentCycle() {
  if (sc_core::sc_start_of_simulation_invoked())
    return sc_core::sc_time_stamp().to_default_time_units();
  else
    return 0;
}
