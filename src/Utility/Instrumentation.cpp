/*
 * Instrumentation.cpp
 *
 *  Created on: 16 Jun 2010
 *      Author: db434
 */

#include "Instrumentation.h"
#include "Debugger.h"
#include "Instrumentation/IPKCache.h"
#include "Instrumentation/Memory.h"
#include "Instrumentation/Network.h"
#include "Instrumentation/Operations.h"
#include "Instrumentation/Stalls.h"
#include "../Datatype/Instruction.h"

void Instrumentation::IPKCacheHit(bool hit) {
  IPKCache::cacheHit(hit);
}

void Instrumentation::memoryRead() {
  Memory::read();
}

void Instrumentation::memoryWrite() {
  Memory::write();
}

void Instrumentation::stalled(int id, bool stalled, int cycle) {
  if(stalled) Stalls::stall(id, cycle);
  else Stalls::unstall(id, cycle);
}

void Instrumentation::idle(int id, bool idle, int cycle) {
  if(idle) Stalls::idle(id, cycle);
  else Stalls::active(id, cycle);
}

void Instrumentation::endExecution() {
  Stalls::endExecution();
}

void Instrumentation::networkTraffic(int startID, int endID, double distance) {
  Network::traffic(startID, endID, distance);
}

void Instrumentation::operation(Instruction inst, bool executed, int id) {
  Operations::operation(InstructionMap::operation(inst.opcode()), executed);
  Debugger::executedInstruction(inst, id);
}

void Instrumentation::printStats() {
  std::cout.precision(3);
  IPKCache::printStats();
  Memory::printStats();
  Network::printStats();
  Operations::printStats();
  Stalls::printStats();
}
