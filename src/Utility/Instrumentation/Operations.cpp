/*
 * Operations.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include "Operations.h"
#include "../InstructionMap.h"

CounterMap<int> Operations::executedOps;
CounterMap<int> Operations::unexecutedOps;
int Operations::numOps = 0;

void Operations::operation(int op, bool executed) {
  int opcopy = op;  // Hack so we can pass a reference

  if(executed) executedOps.increment(opcopy);
  else unexecutedOps.increment(opcopy);

  numOps++;
}

void Operations::printStats() {
  if(numOps > 0) {
    cout << "Operations:" << endl <<
      "  Total: " << numOps << endl <<
      "  Operation\tExecuted \tUnexecuted" << endl;

    for(int i=0; i<InstructionMap::SETCHMAP; i++) {
      if(executedOps[i] > 0 || unexecutedOps[i] > 0) {
        int total = executedOps[i] + unexecutedOps[i];
        string name = InstructionMap::name(i);

        cout << "  ";

        cout.width(14);

        cout << std::left << name << executedOps[i] << " ("
             << asPercentage(executedOps[i],total) << ")\t" << unexecutedOps[i]
             << " (" << asPercentage(unexecutedOps[i],total) << ")" << endl;
      }
    }
  }
}
