/*
 * FIFO.cpp
 *
 *  Created on: 2 May 2012
 *      Author: db434
 */

#include "FIFO.h"

using namespace Instrumentation;

CounterMap<size_t> FIFO::pushes, FIFO::pops, FIFO::instances, FIFO::activeCycles;

void FIFO::push(size_t size) {
  pushes.increment(size);
}

void FIFO::pop(size_t size) {
  pops.increment(size);
}

void FIFO::createdFIFO(size_t size) {
  instances.increment(size);
}

void FIFO::activeCycle(size_t size) {
  activeCycles.increment(size);
}

void FIFO::dumpEventCounts(std::ostream& os) {
  CounterMap<size_t>::iterator it;

  for (it = pushes.begin(); it != pushes.end(); it++) {
    size_t size = it->first;
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
}
