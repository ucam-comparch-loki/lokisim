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

CounterMap<operation_t> Operations::executedOps;
CounterMap<operation_t> Operations::unexecutedOps;
unsigned long long Operations::numOps_ = 0;
unsigned long long Operations::numDecodes_ = 0;

void Operations::decoded(const ComponentID &core, const DecodedInst& dec) {
  // May later care about the operation, since different ones require different
  // decode energies?
  numDecodes_++;
}

void Operations::operation(operation_t op, bool executed) {
  operation_t opcopy = op;  // Hack so we can pass a reference

  if(executed) executedOps.increment(opcopy);
  else unexecutedOps.increment(opcopy);

  numOps_++;
}

unsigned long long Operations::numDecodes()                         {return numDecodes_;}
unsigned long long Operations::numOperations()                      {return numOps_;}
unsigned long long Operations::numOperations(operation_t operation) {return executedOps[operation];}

void Operations::printStats() {
  if (BATCH_MODE)
	cout << "<@GLOBAL>operation_count:" << numOps_ << "</@GLOBAL>" << endl;

  if(numOps_ > 0) {
    cout << "Operations:\n" <<
      "  Total: " << numOps_ << "\n" <<
      "  Operations executed:\n";

    int executed = executedOps.numEvents();

    for(int i=0; i<InstructionMap::numInstructions(); i++) {
      operation_t op = static_cast<operation_t>(i);

      if(executedOps[op] > 0 || unexecutedOps[op] > 0) {
        inst_name_t name = InstructionMap::name(op);

        if (BATCH_MODE)
        	cout << "<@SUBTABLE>operations!op_name:" << name << "!exec_count:" << executedOps[op] << "</@SUBTABLE>" << endl;

        cout << "    ";
        cout.width(14);

        cout << std::left << name << executedOps[op]
             << "\t(" << percentage(executedOps[op],executed) << ")\n";
      }
    }
  }
}
