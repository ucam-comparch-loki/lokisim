/*
 * Registers.cpp
 *
 *  Created on: 21 Jan 2011
 *      Author: db434
 */

#include "Registers.h"

int Registers::numReads = 0;
int Registers::numWrites = 0;
int Registers::numForwards = 0;

// TODO: determine which reads/writes were wasted, and could have been replaced
// by explicit data forwarding.
// This is the case when a value is forwarded, and then that register is written
// before it is read (or not read at all).

void Registers::read(ComponentID core, RegisterIndex reg) {
  numReads++;
}

void Registers::write(ComponentID core, RegisterIndex reg) {
  numWrites++;
}

void Registers::forward(ComponentID core, RegisterIndex reg) {
  numForwards++;
}

void Registers::printStats() {
  cout <<
    "Registers:\n" <<
    "  Reads:    " << numReads << "\n" <<
    "  Writes:   " << numWrites << "\n" <<
    "  Forwards: " << numForwards << "\t(" << asPercentage(numForwards,numReads) << ")\n";
}
