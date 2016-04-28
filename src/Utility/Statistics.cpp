/*
 * Statistics.cpp
 *
 *  Created on: 28 Jan 2011
 *      Author: db434
 */

#include "Statistics.h"
#include "Instrumentation.h"
#include "Instrumentation/IPKCache.h"
#include "Instrumentation/MainMemory.h"
#include "Instrumentation/MemoryBank.h"
#include "Instrumentation/Network.h"
#include "Instrumentation/Operations.h"
#include "Instrumentation/Registers.h"
#include "Instrumentation/Stalls.h"

using namespace Instrumentation;

int Statistics::getStat(const std::string& statName, int parameter) {
  if (statName == "execution_time")        return executionTime();
  else if (statName == "current_cycle")    return currentCycle();
  else if (statName == "l0_tag_checks")    return l0TagChecks();
  else if (statName == "l0_hits")          return l0Hits();
  else if (statName == "l0_misses")        return l0Misses();
  else if (statName == "l0_reads")         return l0Reads();
  else if (statName == "l0_writes")        return l0Writes();
  else if (statName == "l1_reads")         return l1Reads();
  else if (statName == "l1 writes")        return l1Writes();
  else if (statName == "decodes")          return decodes();
  else if (statName == "reg_reads")        return registerReads();
  else if (statName == "reg_writes")       return registerWrites();
  else if (statName == "data_forwards")    return dataForwards();
  else if (statName == "operations") {
    if (parameter != -1)                   return operations((opcode_t)parameter);
    else                                   return operations();
  }
  else if (statName == "cycles_active")    return cyclesActive(parameter);
  else if (statName == "cycles_idle")      return cyclesIdle(parameter);
  else if (statName == "cycles_stalled")   return cyclesStalled(parameter);
  else std::cerr << "Unknown statistic requested: " << statName << "\n";

  return 0;
}

int Statistics::currentCycle() {
  if (sc_core::sc_end_of_simulation_invoked()) return executionTime();
  else return Instrumentation::currentCycle();
}

int Statistics::executionTime()         {return Stalls::executionTime();}

int Statistics::decodes()               {return Operations::numDecodes();}

int Statistics::operations()            {return Operations::numOperations();}
int Statistics::operations(opcode_t op) {return Operations::numOperations(op);}

int Statistics::registerReads()         {return Registers::numReads();}
int Statistics::registerWrites()        {return Registers::numWrites();}
int Statistics::dataForwards()          {return Registers::numForwards();}

int Statistics::l0TagChecks()           {return IPKCache::numTagChecks();}
int Statistics::l0Hits()                {return IPKCache::numHits();}
int Statistics::l0Misses()              {return IPKCache::numMisses();}
int Statistics::l0Reads()               {return IPKCache::numReads();}
int Statistics::l0Writes()              {return IPKCache::numWrites();}

int Statistics::l1Reads()               {return MemoryBank::totalReads();}
int Statistics::l1Writes()              {return MemoryBank::totalWrites();}

int Statistics::cyclesActive(int core)  {
  uint tile = core / NUM_COMPUTE_TILES;
  uint tileX = tile % COMPUTE_TILE_COLUMNS;
  uint tileY = tile / COMPUTE_TILE_COLUMNS;
  uint position = core % NUM_COMPUTE_TILES;
  return Stalls::cyclesActive(ComponentID(tileX, tileY, position));
}

int Statistics::cyclesIdle(int core)    {
  uint tile = core / NUM_COMPUTE_TILES;
  uint tileX = tile % COMPUTE_TILE_COLUMNS;
  uint tileY = tile / COMPUTE_TILE_COLUMNS;
  uint position = core % NUM_COMPUTE_TILES;
  return Stalls::cyclesIdle(ComponentID(tileX, tileY, position));
}

int Statistics::cyclesStalled(int core) {
  uint tile = core / NUM_COMPUTE_TILES;
  uint tileX = tile % COMPUTE_TILE_COLUMNS;
  uint tileY = tile / COMPUTE_TILE_COLUMNS;
  uint position = core % NUM_COMPUTE_TILES;
  return Stalls::cyclesStalled(ComponentID(tileX, tileY, position));
}
