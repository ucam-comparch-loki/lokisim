/*
 * FIFO.cpp
 *
 *  Created on: 2 May 2012
 *      Author: db434
 */

#include "FIFO.h"
#include "../Parameters.h"

using namespace Instrumentation;

CounterMap<size_t> FIFO::pushes, FIFO::pops, FIFO::instances, FIFO::activeCycles;

void FIFO::push(size_t size) {
  if (!Instrumentation::collectingStats()) return;

  pushes.increment(size);
}

void FIFO::pop(size_t size) {
  if (!Instrumentation::collectingStats()) return;

  pops.increment(size);
}

void FIFO::createdFIFO(size_t size) {
  instances.increment(size);
}

void FIFO::activeCycle(size_t size) {
  if (!Instrumentation::collectingStats()) return;

  activeCycles.increment(size);
}

void FIFO::dumpEventCounts(std::ostream& os, const chip_parameters_t& params) {
  CounterMap<size_t>::iterator it;
  size_t ipkFIFOSize = params.tile.core.ipkFIFO.size;

  for (it = pushes.begin(); it != pushes.end(); it++) {
    size_t size = it->first;
    if (size == ipkFIFOSize)
      break;    // IPK FIFO has a different implementation, so separate it

    count_t numPushes = it->second;
    count_t numPops = pops[size];

    // TODO: parameterise width too
    os << "<fifo entries=\"" << size << "\" width=\"32\">\n"
       << xmlNode("instances", instances[size]) << "\n"
       << xmlNode("active", activeCycles[size]) << "\n"
       << xmlNode("push", numPushes)            << "\n"
       << xmlNode("pop", numPops)               << "\n"
       << xmlEnd("fifo")                        << "\n";
  }

  os << "<ipkfifo entries=\"" << ipkFIFOSize << "\">\n"
     << xmlNode("instances", instances[ipkFIFOSize]) << "\n"
     << xmlNode("active", activeCycles[ipkFIFOSize]) << "\n"
     << xmlNode("push", pushes[ipkFIFOSize])         << "\n"
     << xmlNode("pop", pops[ipkFIFOSize])            << "\n"
     << xmlEnd("ipkfifo")                              << "\n";
}
