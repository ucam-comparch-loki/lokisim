/*
 * Statistics.cpp
 *
 *  Created on: 28 Jan 2011
 *      Author: db434
 */

#include "Statistics.h"
#include "Instrumentation.h"
#include "Instrumentation/Energy.h"
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
  Energy::printStats();
  Stalls::printStats();
}

int Statistics::getStat(const std::string& statName, int parameter) {
  if(statName == "execution_time")        return executionTime();
  else if(statName == "energy")           return energy();
  else if(statName == "fj_per_op")        return fJPerOp();
  else if(statName == "l0_tag_checks")    return l0TagChecks();
  else if(statName == "l0_hits")          return l0Hits();
  else if(statName == "l0_misses")        return l0Misses();
  else if(statName == "l0_reads")         return l0Reads();
  else if(statName == "l0_writes")        return l0Writes();
  else if(statName == "decodes")          return decodes();
  else if(statName == "reg_reads")        return registerReads();
  else if(statName == "reg_writes")       return registerWrites();
  else if(statName == "data_forwards")    return dataForwards();
  else if(statName == "operations") {
    if(parameter != -1)                   return operations(parameter);
    else                                  return operations();
  }
  else if(statName == "l1_tag_checks")    return l1TagChecks();
  else if(statName == "l1_hits")          return l1Hits();
  else if(statName == "l1_misses")        return l1Misses();
  else if(statName == "l1_reads")         return l1Reads();
  else if(statName == "l1_writes")        return l1Writes();
  else if(statName == "network_distance") return (int)networkDistance();
  else if(statName == "cycles_active")    return cyclesActive(parameter);
  else if(statName == "cycles_idle")      return cyclesIdle(parameter);
  else if(statName == "cycles_stalled")   return cyclesStalled(parameter);
  else std::cerr << "Unknown statistic requested: " << statName << "\n";

  return 0;
}

int Statistics::executionTime()      {return Stalls::executionTime();}
int Statistics::energy()             {return Energy::totalEnergy();}
int Statistics::fJPerOp()            {return Energy::pJPerOp()*1000;}

int Statistics::decodes()            {return Operations::numDecodes();}

int Statistics::operations()         {return Operations::numOperations();}
int Statistics::operations(int operation) {return Operations::numOperations(operation);}

int Statistics::registerReads()      {return Registers::numReads();}
int Statistics::registerWrites()     {return Registers::numWrites();}
int Statistics::dataForwards()       {return Registers::numForwards();}
int Statistics::stallRegUses()       {return Registers::stallRegUses();}

int Statistics::l0TagChecks()        {return IPKCache::numTagChecks();}
int Statistics::l0Hits()             {return IPKCache::numHits();}
int Statistics::l0Misses()           {return IPKCache::numMisses();}
int Statistics::l0Reads()            {return IPKCache::numReads();}
int Statistics::l0Writes()           {return IPKCache::numWrites();}

int Statistics::l1TagChecks()        {return Memory::numTagChecks();}
int Statistics::l1Hits()             {return Memory::numHits();}
int Statistics::l1Misses()           {return Memory::numMisses();}
int Statistics::l1Reads()            {return Memory::numReads();}
int Statistics::l1Writes()           {return Memory::numWrites();}

double Statistics::networkDistance() {return Network::totalDistance();}

int Statistics::cyclesActive(ComponentID core)  {return Stalls::cyclesActive(core);}
int Statistics::cyclesIdle(ComponentID core)    {return Stalls::cyclesIdle(core);}
int Statistics::cyclesStalled(ComponentID core) {return Stalls::cyclesStalled(core);}
