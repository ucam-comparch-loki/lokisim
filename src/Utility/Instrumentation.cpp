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
#include "Instrumentation/Stalls.h"
#include "../Datatype/AddressedWord.h"
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
}

void Instrumentation::dumpEventCounts(std::ostream& os) {
  os << "<lokitrace>\n\n";

  os << "<invocation>" << Arguments::invocation() << "</invocation>\n";
  os << "\n";
  os << "<time>" << time(NULL) << "</time>\n";
  os << "\n";
  Parameters::printParametersXML(os);
  os << "\n";
  os << "<cycles>" << Stalls::executionTime() << "</cycles>\n";
  os << "\n";

  // Program name?
  // Cycles each component was active

  ChannelMap::dumpEventCounts(os);    os << "\n";
  FIFO::dumpEventCounts(os);          os << "\n";
  IPKCache::dumpEventCounts(os);      os << "\n";
  MemoryBank::dumpEventCounts(os);    os << "\n";
  Network::dumpEventCounts(os);       os << "\n";
  Operations::dumpEventCounts(os);    os << "\n";
  PipelineReg::dumpEventCounts(os);   os << "\n";
  Registers::dumpEventCounts(os);     os << "\n";

  os << "</lokitrace>\n";
}

void Instrumentation::decoded(const ComponentID& core, const DecodedInst& dec) {
  Operations::decoded(core, dec);

  if(TRACE) {

    MemoryAddr location = dec.location();
    std::cout << "0x";
    std::cout.width(8);
    std::cout << std::hex << std::setfill('0') << location << endl;


//    std::cout << dec << endl;
  }
}

void Instrumentation::memorySetMode(int bank, bool isCache, uint setCount, uint wayCount, uint lineSize)	{MemoryBank::setMode(bank, isCache, setCount, wayCount, lineSize);}

void Instrumentation::memoryReadWord(int bank, MemoryAddr address, bool isMiss)	{MemoryBank::readWord(bank, address, isMiss);}
void Instrumentation::memoryReadHalfWord(int bank, MemoryAddr address, bool isMiss)	{MemoryBank::readHalfWord(bank, address, isMiss);}
void Instrumentation::memoryReadByte(int bank, MemoryAddr address, bool isMiss) {MemoryBank::readByte(bank, address, isMiss);}

void Instrumentation::memoryWriteWord(int bank, MemoryAddr address, bool isMiss) {MemoryBank::writeWord(bank, address, isMiss);}
void Instrumentation::memoryWriteHalfWord(int bank, MemoryAddr address, bool isMiss) {MemoryBank::writeHalfWord(bank, address, isMiss);}
void Instrumentation::memoryWriteByte(int bank, MemoryAddr address, bool isMiss) {MemoryBank::writeByte(bank, address, isMiss);}

void Instrumentation::memoryInitiateIPKRead(int bank, bool isHandOff)										{MemoryBank::initiateIPKRead(bank, isHandOff);}
void Instrumentation::memoryInitiateBurstRead(int bank, bool isHandOff)										{MemoryBank::initiateBurstRead(bank, isHandOff);}
void Instrumentation::memoryInitiateBurstWrite(int bank, bool isHandOff)									{MemoryBank::initiateBurstWrite(bank, isHandOff);}

void Instrumentation::memoryReadIPKWord(int bank, MemoryAddr address, bool isMiss) {MemoryBank::readIPKWord(bank, address, isMiss);}

void Instrumentation::memoryReadBurstWord(int bank, MemoryAddr address, bool isMiss)						{MemoryBank::readBurstWord(bank, address, isMiss);}
void Instrumentation::memoryWriteBurstWord(int bank, MemoryAddr address, bool isMiss)						{MemoryBank::writeBurstWord(bank, address, isMiss);}

void Instrumentation::memoryReplaceCacheLine(int bank, bool isValid, bool isDirty)							{MemoryBank::replaceCacheLine(bank, isValid, isDirty);}

void Instrumentation::memoryRingHandOff(int bank)															{MemoryBank::ringHandOff(bank);}
void Instrumentation::memoryRingPassThrough(int bank)														{MemoryBank::ringPassThrough(bank);}

// Record that background memory was read from.
void Instrumentation::backgroundMemoryRead(MemoryAddr address, uint32_t count) {BackgroundMemory::read(address, count);}
// Record that background memory was written to.
void Instrumentation::backgroundMemoryWrite(MemoryAddr address, uint32_t count)								{BackgroundMemory::write(address, count);}


void Instrumentation::idle(const ComponentID& id, bool idle) {
  if(idle) Stalls::idle(id, currentCycle());
  else Stalls::active(id, currentCycle());
}

void Instrumentation::endExecution() {
  Stalls::endExecution();

  // Only end simulation if we aren't using the debugger: we may still want to
  // probe memory contents.
  if(!Debugger::usingDebugger) sc_stop();
}

bool Instrumentation::executionFinished() {
  return Stalls::executionFinished();
}

void Instrumentation::executed(const ComponentID& id, const DecodedInst& inst, bool executed) {
  if (ENERGY_TRACE)
    Operations::executed(id, inst, executed);

  if (Debugger::mode == Debugger::DEBUGGER)
    Debugger::executedInstruction(inst, id, executed);
}

unsigned long Instrumentation::currentCycle() {
  return sc_core::sc_time_stamp().to_default_time_units();
}
