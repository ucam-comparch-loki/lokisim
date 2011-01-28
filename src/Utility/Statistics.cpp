/*
 * Statistics.cpp
 *
 *  Created on: 28 Jan 2011
 *      Author: db434
 */

#include "Statistics.h"
#include "Instrumentation.h"
#include "Instrumentation/IPKCache.h"
#include "Instrumentation/Memory.h"
#include "Instrumentation/Network.h"
#include "Instrumentation/Operations.h"
#include "Instrumentation/Registers.h"
#include "Instrumentation/Stalls.h"

void Statistics::printStats() {
  std::cout.precision(3);
  IPKCache::printStats();
  Memory::printStats();
  Network::printStats();
  Operations::printStats();
  Registers::printStats();
  Stalls::printStats();
}

int Statistics::getStat(const std::string& statName, int parameter) {
  if(statName == "execution_time")        return executionTime();
  else if(statName == "energy")           return energy();
  else if(statName == "tag_checks")       return tagChecks();
  else if(statName == "cache_hits")       return cacheHits();
  else if(statName == "cache_misses")     return cacheMisses();
  else if(statName == "cache_reads")      return cacheReads();
  else if(statName == "cache_writes")     return cacheWrites();
  else if(statName == "decodes")          return decodes();
  else if(statName == "reg_reads")        return registerReads();
  else if(statName == "reg_writes")       return registerWrites();
  else if(statName == "data_forwards")    return dataForwards();
  else if(statName == "operations") {
    if(parameter != -1)                   return operations(parameter);
    else                                  return operations();
  }
  else if(statName == "mem_reads")        return memoryReads();
  else if(statName == "mem_writes")       return memoryWrites();
  else if(statName == "network_distance") return (int)networkDistance();
  else if(statName == "cycles_active")    return cyclesActive(parameter);
  else if(statName == "cycles_idle")      return cyclesIdle(parameter);
  else if(statName == "cycles_stalled")   return cyclesStalled(parameter);
  else std::cerr << "Unknown statistic requested: " << statName << "\n";

  return 0;
}

int Statistics::executionTime()  {return Stalls::executionTime();}
int Statistics::energy() {
  return 0;
//  return Energy::totalEnergy();
}

int Statistics::tagChecks()      {return IPKCache::numTagChecks();}
int Statistics::cacheHits()      {return IPKCache::numHits();}
int Statistics::cacheMisses()    {return IPKCache::numMisses();}
int Statistics::cacheReads()     {return IPKCache::numReads();}
int Statistics::cacheWrites()    {return IPKCache::numWrites();}

int Statistics::decodes()        {return Operations::numDecodes();}

int Statistics::registerReads()  {return Registers::numReads();}
int Statistics::registerWrites() {return Registers::numWrites();}
int Statistics::dataForwards()   {return Registers::numForwards();}

int Statistics::operations()     {return Operations::numOperations();}
int Statistics::operations(int operation) {return Operations::numOperations(operation);}

int Statistics::memoryReads()    {return Memory::numReads();}
int Statistics::memoryWrites()   {return Memory::numWrites();}

double Statistics::networkDistance() {return Network::totalDistance();}

int Statistics::cyclesActive(ComponentID core)  {return Stalls::cyclesActive(core);}
int Statistics::cyclesIdle(ComponentID core)    {return Stalls::cyclesIdle(core);}
int Statistics::cyclesStalled(ComponentID core) {return Stalls::cyclesStalled(core);}
