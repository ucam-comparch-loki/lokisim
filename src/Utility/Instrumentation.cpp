/*
 * Instrumentation.cpp
 *
 *  Created on: 16 Jun 2010
 *      Author: db434
 */

#include <iomanip>
#include "Instrumentation.h"
#include "StartUp/CodeLoader.h"
#include "Debugger.h"
#include "Instrumentation/BackgroundMemory.h"
#include "Instrumentation/Energy.h"
#include "Instrumentation/IPKCache.h"
#include "Instrumentation/Memory.h"
#include "Instrumentation/MemoryBank.h"
#include "Instrumentation/Network.h"
#include "Instrumentation/Operations.h"
#include "Instrumentation/Registers.h"
#include "Instrumentation/Stalls.h"
#include "../Datatype/DecodedInst.h"

void Instrumentation::l0TagCheck(const ComponentID& core, bool hit) {
  IPKCache::tagCheck(core, hit);
}

void Instrumentation::l0Read(const ComponentID& core) {
  IPKCache::read(core);
}

void Instrumentation::l0Write(const ComponentID& core) {
  IPKCache::write(core);
}

void Instrumentation::decoded(const ComponentID& core, const DecodedInst& dec) {
  Operations::decoded(core, dec);

  if(TRACE) {
	/*
    MemoryAddr location = dec.location();
    std::cout << "0x";
    std::cout.width(8);
    std::cout << std::hex << std::setfill('0') << location << endl;
    */

	std::cout << dec << endl;
  }
}

void Instrumentation::dataForwarded(const ComponentID& core, RegisterIndex reg) {
  Registers::forward(core, reg);
}

void Instrumentation::registerRead(const ComponentID& core, RegisterIndex reg) {
  Registers::read(core, reg);
}

void Instrumentation::registerWrite(const ComponentID& core, RegisterIndex reg) {
  Registers::write(core, reg);
}

void Instrumentation::stallRegUse(const ComponentID& core) {
  Registers::stallReg(core);
}

void Instrumentation::l1TagCheck(MemoryAddr address, bool hit) {
  Memory::tagCheck(address, hit);
}

void Instrumentation::l1Read(MemoryAddr address, bool isInstruction) {
  Memory::read(address, isInstruction);
}

void Instrumentation::l1Write(MemoryAddr address) {
  Memory::write(address);
}

void Instrumentation::memorySetMode(int bank, bool isCache, uint setCount, uint wayCount, uint lineSize)	{MemoryBank::setMode(bank, isCache, setCount, wayCount, lineSize);}

void Instrumentation::memoryReadWord(int bank, MemoryAddr address, bool isMiss)								{MemoryBank::readWord(bank, address, isMiss);}
void Instrumentation::memoryReadHalfWord(int bank, MemoryAddr address, bool isMiss)							{MemoryBank::readHalfWord(bank, address, isMiss);}
void Instrumentation::memoryReadByte(int bank, MemoryAddr address, bool isMiss)								{MemoryBank::readByte(bank, address, isMiss);}

void Instrumentation::memoryWriteWord(int bank, MemoryAddr address, bool isMiss)							{MemoryBank::writeWord(bank, address, isMiss);}
void Instrumentation::memoryWriteHalfWord(int bank, MemoryAddr address, bool isMiss)						{MemoryBank::writeHalfWord(bank, address, isMiss);}
void Instrumentation::memoryWriteByte(int bank, MemoryAddr address, bool isMiss)							{MemoryBank::writeByte(bank, address, isMiss);}

void Instrumentation::memoryInitiateIPKRead(int bank, bool isHandOff)										{MemoryBank::initiateIPKRead(bank, isHandOff);}
void Instrumentation::memoryInitiateBurstRead(int bank, bool isHandOff)										{MemoryBank::initiateBurstRead(bank, isHandOff);}
void Instrumentation::memoryInitiateBurstWrite(int bank, bool isHandOff)									{MemoryBank::initiateBurstWrite(bank, isHandOff);}

void Instrumentation::memoryReadIPKWord(int bank, MemoryAddr address, bool isMiss)							{MemoryBank::readIPKWord(bank, address, isMiss);}
void Instrumentation::memoryReadBurstWord(int bank, MemoryAddr address, bool isMiss)						{MemoryBank::readBurstWord(bank, address, isMiss);}
void Instrumentation::memoryWriteBurstWord(int bank, MemoryAddr address, bool isMiss)						{MemoryBank::writeBurstWord(bank, address, isMiss);}

void Instrumentation::memoryReplaceCacheLine(int bank, bool isValid, bool isDirty)							{MemoryBank::replaceCacheLine(bank, isValid, isDirty);}

void Instrumentation::memoryRingHandOff(int bank)															{MemoryBank::ringHandOff(bank);}
void Instrumentation::memoryRingPassThrough(int bank)														{MemoryBank::ringPassThrough(bank);}

// Record that background memory was read from.
void Instrumentation::backgroundMemoryRead(MemoryAddr address, uint32_t count)								{BackgroundMemory::read(address, count);}

// Record that background memory was written to.
void Instrumentation::backgroundMemoryWrite(MemoryAddr address, uint32_t count)								{BackgroundMemory::write(address, count);}

void Instrumentation::stalled(const ComponentID& id, bool stalled, int reason) {
  if(stalled) Stalls::stall(id, currentCycle(), reason);
  else Stalls::unstall(id, currentCycle());
}

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

void Instrumentation::networkTraffic(const ChannelID& startID, const ChannelID& endID) {
	Network::traffic(startID.getComponentID(), endID.getComponentID());
}

// Remove some parameters? The distance and bitsSwitched are the only
// interesting ones.
void Instrumentation::networkActivity(const ComponentID& network, ChannelIndex source,
    ChannelIndex destination, double distance, int bitsSwitched) {
  Network::activity(network, source, destination, distance, bitsSwitched);
}

void Instrumentation::operation(const ComponentID& id, const DecodedInst& inst, bool executed) {
  Operations::operation(inst.operation(), executed);

  if(Debugger::mode == Debugger::DEBUGGER)
    Debugger::executedInstruction(inst, id, executed);
}

int Instrumentation::currentCycle() {
  return sc_core::sc_time_stamp().to_default_time_units();
}

void Instrumentation::loadPowerLibrary(const std::string& filename) {
  Energy::loadLibrary(filename);
}
