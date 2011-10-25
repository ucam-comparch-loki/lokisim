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

CounterMap<operation_key> Operations::executedOps;
CounterMap<operation_key> Operations::unexecutedOps;
unsigned long long Operations::numOps_ = 0;
unsigned long long Operations::numDecodes_ = 0;

void Operations::decoded(const ComponentID& core, const DecodedInst& dec) {
  // May later care about the operation, since different ones require different
  // decode energies?
  numDecodes_++;
}

void Operations::executed(const ComponentID& core, const DecodedInst& dec, bool executed) {
  operation_key key;
  key.opcode = dec.opcode();
  key.function = dec.function();

  if(executed) executedOps.increment(key);
  else unexecutedOps.increment(key);

  numOps_++;
}

unsigned long long Operations::numDecodes()               {return numDecodes_;}
unsigned long long Operations::numOperations()            {return numOps_;}

unsigned long long Operations::numOperations(opcode_t op, function_t function) {
  operation_key key;
  key.opcode = op;
  key.function = function;
  return executedOps[key];
}

void Operations::printStats() {
  if (BATCH_MODE)
	cout << "<@GLOBAL>operation_count:" << numOps_ << "</@GLOBAL>" << endl;

  if(numOps_ > 0) {
    cout << "Operations:\n" <<
      "  Total: " << numOps_ << "\n" <<
      "  Operations executed:\n";

    int executed = executedOps.numEvents();

    CounterMap<operation_key>::iterator it;

    for(it=executedOps.begin(); it != executedOps.end(); it++) {
      operation_key op = it->first;

      if(executedOps[op] > 0 || unexecutedOps[op] > 0) {
        const inst_name_t& name = InstructionMap::name(op.opcode, op.function);

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
