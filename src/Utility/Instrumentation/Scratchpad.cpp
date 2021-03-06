/*
 * Scratchpad.cpp
 *
 *  Created on: 17 Jan 2013
 *      Author: db434
 */

#include "Scratchpad.h"
#include "../Parameters.h"

using namespace Instrumentation;

count_t Scratchpad::reads = 0;
count_t Scratchpad::writes = 0;

void    Scratchpad::read()      {
  if (!Instrumentation::collectingStats()) return;
  reads++;
}

void    Scratchpad::write()     {
  if (!Instrumentation::collectingStats()) return;
  writes++;
}

count_t Scratchpad::numReads()  {return reads;}
count_t Scratchpad::numWrites() {return writes;}

void    Scratchpad::dumpEventCounts(std::ostream& os, const chip_parameters_t& params) {
  os << "<scratchpad entries=\"" << params.tile.core.scratchpad.size << "\">\n"
     << xmlNode("instances", params.totalCores())   << "\n"
     << xmlNode("active", numReads() + numWrites()) << "\n"
     << xmlNode("read", numReads())                 << "\n"
     << xmlNode("write", numWrites())               << "\n"
     << xmlEnd("scratchpad")                        << "\n";
}
