/*
 * FIFO.cpp
 *
 *  Created on: 2 May 2012
 *      Author: db434
 */

#include "FIFO.h"

using namespace Instrumentation;

CounterMap<size_t> FIFO::pushes, FIFO::pops;

void FIFO::push(size_t size) {
  pushes.increment(size);
}

void FIFO::pop(size_t size) {
  pops.increment(size);
}

void FIFO::dumpEventCounts(std::ostream& os) {
  CounterMap<size_t>::iterator it;

  for (it = pushes.begin(); it != pushes.end(); it++) {
    size_t size = it->first;
    count_t numPushes = it->second;
    count_t numPops = pops[size];

    // For the moment, we assume that all small FIFOs are pipeline registers.
    // There is a good chance that this is true, but it might not be.
    // TODO: parameterise width too
    if (size <= 2) {
      os << "<pipeline_register width=\"32\">\n"
         << xmlNode("read", numPops) << "\n"
         << xmlNode("write", numPushes) << "\n"
         << xmlEnd("pipeline_register") << "\n";
    }
    else {
      os << "<fifo entries=\"" << size << "\" width=\"32\">\n"
         << xmlNode("push", numPushes) << "\n"
         << xmlNode("pop", numPops) << "\n"
         << xmlEnd("fifo") << "\n";
    }
  }
}
