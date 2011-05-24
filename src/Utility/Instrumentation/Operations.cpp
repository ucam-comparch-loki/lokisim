/*
 * Operations.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include "Operations.h"
#include "../InstructionMap.h"
#include "../../Datatype/ComponentID.h"
#include "../../Datatype/DecodedInst.h"

CounterMap<int> Operations::executedOps;
CounterMap<int> Operations::unexecutedOps;
int Operations::numOps_ = 0;
int Operations::numDecodes_ = 0;

void Operations::decoded(const ComponentID &core, const DecodedInst& dec) {
  // May later care about the operation, since different ones require different
  // decode energies?
  numDecodes_++;
}

void Operations::operation(int op, bool executed) {
  int opcopy = op;  // Hack so we can pass a reference

  if(executed) executedOps.increment(opcopy);
  else unexecutedOps.increment(opcopy);

  numOps_++;
}

int Operations::numDecodes()                 {return numDecodes_;}
int Operations::numOperations()              {return numOps_;}
int Operations::numOperations(int operation) {return executedOps[operation];}

void Operations::printStats() {
  if (BATCH_MODE)
	cout << "<@GLOBAL>operation_count:" << numOps_ << "</@GLOBAL>" << endl;

  if(numOps_ > 0) {
    cout << "Operations:\n" <<
      "  Total: " << numOps_ << "\n" <<
      "  Operations executed:\n";

    int executed = executedOps.numEvents();

    for(int i=0; i<InstructionMap::SYSCALL; i++) {
      if(executedOps[i] > 0 || unexecutedOps[i] > 0) {
        string name = InstructionMap::name(i);

        if (BATCH_MODE)
        	cout << "<@SUBTABLE>operations!op_name:" << name << "!exec_count:" << executedOps[i] << "</@SUBTABLE>" << endl;

        cout << "    ";
        cout.width(14);

        cout << std::left << name << executedOps[i]
             << "\t(" << percentage(executedOps[i],executed) << ")\n";
      }
    }
  }
}
