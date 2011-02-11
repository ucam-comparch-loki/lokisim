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
#include "Instrumentation/Energy.h"
#include "Instrumentation/IPKCache.h"
#include "Instrumentation/Memory.h"
#include "Instrumentation/Network.h"
#include "Instrumentation/Operations.h"
#include "Instrumentation/Registers.h"
#include "Instrumentation/Stalls.h"
#include "../Datatype/DecodedInst.h"

void Instrumentation::l0TagCheck(ComponentID core, bool hit) {
  IPKCache::tagCheck(core, hit);
}

void Instrumentation::l0Read(ComponentID core) {
  IPKCache::read(core);
}

void Instrumentation::l0Write(ComponentID core) {
  IPKCache::write(core);
}

void Instrumentation::decoded(ComponentID core, const DecodedInst& dec) {
  Operations::decoded(core, dec);

  if(TRACE) {
    MemoryAddr location = dec.location().address();
    std::cout << "0x";
    std::cout.width(8);
    std::cout << std::hex << std::setfill('0') << location << endl;
  }
}

void Instrumentation::dataForwarded(ComponentID core, RegisterIndex reg) {
  Registers::forward(core, reg);
}

void Instrumentation::registerRead(ComponentID core, RegisterIndex reg) {
  Registers::read(core, reg);
}

void Instrumentation::registerWrite(ComponentID core, RegisterIndex reg) {
  Registers::write(core, reg);
}

void Instrumentation::stallRegUse(ComponentID core) {
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

void Instrumentation::stalled(ComponentID id, bool stalled, int reason) {
  if(stalled) Stalls::stall(id, currentCycle(), reason);
  else Stalls::unstall(id, currentCycle());
}

void Instrumentation::idle(ComponentID id, bool idle) {
  if(idle) Stalls::idle(id, currentCycle());
  else Stalls::active(id, currentCycle());
}

void Instrumentation::endExecution() {
  Stalls::endExecution();
}

void Instrumentation::networkTraffic(ChannelID startID, ChannelID endID) {
  Network::traffic(startID/NUM_CORE_OUTPUTS, endID/NUM_CORE_INPUTS);
}

void Instrumentation::networkActivity(ComponentID network, ChannelIndex source,
    ChannelIndex destination, double distance, int bitsSwitched) {
  Network::activity(network, source, destination, distance, bitsSwitched);
}

void Instrumentation::operation(ComponentID id, const DecodedInst& inst, bool executed) {
  Operations::operation(inst.operation(), executed);

  if(CodeLoader::usingDebugger)
    Debugger::executedInstruction(inst, id, executed);
}

int Instrumentation::currentCycle() {
  return sc_core::sc_time_stamp().to_default_time_units();
}

void Instrumentation::loadPowerLibrary(const std::string& filename) {
  Energy::loadLibrary(filename);
}
