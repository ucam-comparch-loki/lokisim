/*
 * Operations.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include "Operations.h"
#include "../InstructionMap.h"
#include "../../Datatype/DecodedInst.h"

CounterMap<int> Operations::executedOps;
CounterMap<int> Operations::unexecutedOps;
int Operations::numOps = 0;
int Operations::numDecodes = 0;

void Operations::decoded(ComponentID core, const DecodedInst& dec) {
  // May later care about the operation, since different ones require different
  // decode energies?
  numDecodes++;
}

void Operations::operation(int op, bool executed) {
  int opcopy = op;  // Hack so we can pass a reference

  if(executed) executedOps.increment(opcopy);
  else unexecutedOps.increment(opcopy);

  numOps++;
}

void Operations::printStats() {
  if(numOps > 0) {
    cout << "Operations:\n" <<
      "  Total: " << numOps << "\n" <<
      "  Operations executed:\n";

    int executed = executedOps.numEvents();

    for(int i=0; i<InstructionMap::SETCHMAP; i++) {
      if(executedOps[i] > 0 || unexecutedOps[i] > 0) {
        string name = InstructionMap::name(i);

        cout << "    ";
        cout.width(14);

        cout << std::left << name << executedOps[i]
             << "\t(" << asPercentage(executedOps[i],executed) << ")\n";
      }
    }
  }
}
