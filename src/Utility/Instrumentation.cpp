/*
 * Instrumentation.cpp
 *
 *  Created on: 16 Jun 2010
 *      Author: db434
 */

#include "Instrumentation.h"
#include "StartUp/CodeLoader.h"
#include "Debugger.h"
#include "Instrumentation/IPKCache.h"
#include "Instrumentation/Memory.h"
#include "Instrumentation/Network.h"
#include "Instrumentation/Operations.h"
#include "Instrumentation/Stalls.h"
#include "../Datatype/DecodedInst.h"

void Instrumentation::IPKCacheHit(bool hit) {
  IPKCache::cacheHit(hit);
}

void Instrumentation::memoryRead(MemoryAddr address, bool isInstruction) {
  Memory::read(address, isInstruction);
}

void Instrumentation::memoryWrite(MemoryAddr address) {
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
  Network::traffic(startID/NUM_CLUSTER_OUTPUTS, endID/NUM_CLUSTER_INPUTS);
}

void Instrumentation::networkActivity(ComponentID network, ChannelIndex source,
    ChannelIndex destination, double distance, int bitsSwitched) {
  Network::activity(network, source, destination, distance, bitsSwitched);
}

void Instrumentation::operation(DecodedInst inst, bool executed, ComponentID id) {
  Operations::operation(inst.operation(), executed);

  if(CodeLoader::usingDebugger)
    Debugger::executedInstruction(inst, id, executed);
}

void Instrumentation::printStats() {
  std::cout.precision(3);
  IPKCache::printStats();
  Memory::printStats();
  Network::printStats();
  Operations::printStats();
  Stalls::printStats();
}

int Instrumentation::currentCycle() {
  return sc_core::sc_time_stamp().to_default_time_units();
}
