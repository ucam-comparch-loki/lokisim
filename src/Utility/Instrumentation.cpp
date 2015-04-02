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
#include "Instrumentation/BackgroundMemory.h"
#include "Instrumentation/ChannelMap.h"
#include "Instrumentation/FIFO.h"
#include "Instrumentation/IPKCache.h"
#include "Instrumentation/MemoryBank.h"
#include "Instrumentation/Network.h"
#include "Instrumentation/Operations.h"
#include "Instrumentation/PipelineReg.h"
#include "Instrumentation/Registers.h"
#include "Instrumentation/Scratchpad.h"
#include "Instrumentation/Stalls.h"
#include "../Datatype/DecodedInst.h"

void Instrumentation::initialise() {
  ChannelMap::init();
  FIFO::init();
  IPKCache::init();
  MemoryBank::init();
  Network::init();
  Operations::init();
  PipelineReg::init();
  Registers::init();
  Scratchpad::init();
  Stalls::init();
}

void Instrumentation::end() {
  ChannelMap::end();
  FIFO::end();
  IPKCache::end();
  MemoryBank::end();
  Network::end();
  Operations::end();
  PipelineReg::end();
  Registers::end();
  Scratchpad::end();
  Stalls::end();
}

void Instrumentation::dumpEventCounts(std::ostream& os) {
  os << "<lokitrace>\n\n";

  os << "<invocation>" << Arguments::invocation() << "</invocation>\n";
  os << "\n";
  os << "<time>" << time(NULL) << "</time>\n";
  os << "\n";
  Parameters::printParametersXML(os);
  os << "\n";
  os << "<cycles>" << Stalls::cyclesLogged() << "</cycles>\n";
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
  Operations::printSummary();
}

bool Instrumentation::haveEnergyData() {
  // Assume that if we have collected energy data, at least one instruction
  // has been executed.
  return Stalls::cyclesLogged() > 0;
}

void Instrumentation::startEventLog() {
  // Stalls is currently the only component which needs notification: all
  // event collection is predicated on the ENERGY_TRACE value. Stalls, however,
  // counts cycles, so needs to be told when to suspend its counting.
  Stalls::startLogging();
}

void Instrumentation::stopEventLog() {
  // Stalls is currently the only component which needs notification: all
  // event collection is predicated on the ENERGY_TRACE value. Stalls, however,
  // counts cycles, so needs to be told when to suspend its counting.
  Stalls::stopLogging();
}

void Instrumentation::decoded(const ComponentID& core, const DecodedInst& dec) {
  Operations::decoded(core, dec);

  if (TRACE) {

    MemoryAddr location = dec.location();
    std::cout << "0x";
    std::cout.width(8);
    std::cout << std::hex << std::setfill('0') << location << endl;


//    std::cout << dec << endl;
  }
}

// Record that background memory was read from.
void Instrumentation::backgroundMemoryRead(MemoryAddr address, uint32_t count) {BackgroundMemory::read(address, count);}
// Record that background memory was written to.
void Instrumentation::backgroundMemoryWrite(MemoryAddr address, uint32_t count)								{BackgroundMemory::write(address, count);}


void Instrumentation::idle(const ComponentID id, bool idle) {
  if (idle) Stalls::idle(id, currentCycle());
  else Stalls::active(id, currentCycle());
}

void Instrumentation::endExecution() {
  Stalls::endExecution();

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
