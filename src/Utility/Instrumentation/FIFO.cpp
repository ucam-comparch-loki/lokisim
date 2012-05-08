/*
 * FIFO.cpp
 *
 *  Created on: 2 May 2012
 *      Author: db434
 */

#include "FIFO.h"

CounterMap<size_t> FIFO::pushes, FIFO::pops;

void FIFO::init() {
  // Do nothing.
}

void FIFO::end() {
  // Do nothing.
}

void FIFO::push(size_t size) {
  pushes.increment(size);
}

void FIFO::pop(size_t size) {
  pops.increment(size);
}

void FIFO::dumpEventCounts(std::ostream& os) {
  CounterMap<size_t>::iterator it;

  for(it = pushes.begin(); it != pushes.end(); it++) {
    size_t size = it->first;
    count_t num = it->second;
    os << "fifo" << size << "_push\t" << num << "\n";
  }

  for(it = pops.begin(); it != pops.end(); it++) {
    size_t size = it->first;
    count_t num = it->second;
    os << "fifo" << size << "_pop\t" << num << "\n";
  }
}
