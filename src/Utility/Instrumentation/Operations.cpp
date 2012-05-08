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

CounterMap<opcode_t> Operations::executedOps;
CounterMap<function_t> Operations::executedFns;
count_t Operations::unexecuted;

int32_t* Operations::lastIn1 = NULL;
int32_t* Operations::lastIn2 = NULL;
int32_t* Operations::lastOut = NULL;
function_t* Operations::lastFn = NULL;

count_t Operations::hdIn1 = 0;
count_t Operations::hdIn2 = 0;
count_t Operations::hdOut = 0;
count_t Operations::sameOp = 0;

count_t Operations::numOps_ = 0;
count_t Operations::numDecodes_ = 0;

void Operations::init() {
  lastIn1 = new int32_t[NUM_CORES];
  lastIn2 = new int32_t[NUM_CORES];
  lastOut = new int32_t[NUM_CORES];
  lastFn  = new function_t[NUM_CORES];
}

void Operations::end() {
  delete[] lastIn1; delete[] lastIn2; delete[] lastOut; delete[] lastFn;
}

void Operations::decoded(const ComponentID& core, const DecodedInst& dec) {
  // May later care about the operation, since different ones require different
  // decode energies?
  numDecodes_++;
}

void Operations::executed(const ComponentID& core, const DecodedInst& dec, bool executed) {
  numOps_++;

  if (!executed) {
    unexecuted++;
    return;
  }

  // Basic logging for "ordinary" operations. Extra logging for ALU operations.
  if (dec.opcode() > 1)
    executedOps.increment(dec.opcode());
  else
    executedFns.increment(dec.function());

  if (dec.isALUOperation()) {
    int coreID = core.getGlobalCoreNumber();

    hdIn1 += hammingDistance(dec.operand1(), lastIn1[coreID]);
    hdIn2 += hammingDistance(dec.operand2(), lastIn2[coreID]);
    hdOut += hammingDistance(dec.result(),   lastOut[coreID]);

    if (dec.function() == lastFn[coreID])
      sameOp++;

    lastIn1[coreID] = dec.operand1();
    lastIn2[coreID] = dec.operand2();
    lastOut[coreID] = dec.result();
    lastFn[coreID]  = dec.function();
  }

}

count_t Operations::numDecodes()               {return numDecodes_;}
count_t Operations::numOperations()            {return numOps_;}

count_t Operations::numOperations(opcode_t op, function_t function) {
  // Special case for ALU operations.
  if (op <= 1)
    return executedFns[function];
  else
    return executedOps[op];
}

void Operations::printStats() {
  if (BATCH_MODE)
	cout << "<@GLOBAL>operation_count:" << numOps_ << "</@GLOBAL>" << endl;

  if(numOps_ > 0) {
    cout << "Operations:\n" <<
      "  Total: " << numOps_ << "\n" <<
      "  Operations executed:\n";

    int executed = executedOps.numEvents() + executedFns.numEvents();

    CounterMap<function_t>::iterator it;
    for(it = executedFns.begin(); it != executedFns.end(); it++) {
      function_t fn = it->first;
      const inst_name_t& name = InstructionMap::name((opcode_t)0, fn);

      if (BATCH_MODE)
        cout << "<@SUBTABLE>operations!op_name:" << name << "!exec_count:" << executedFns[fn] << "</@SUBTABLE>" << endl;

      cout << "    ";
      cout.width(14);

      cout << std::left << name << executedFns[fn]
           << "\t(" << percentage(executedFns[fn],executed) << ")\n";
    }

    CounterMap<opcode_t>::iterator it2;
    for(it2 = executedOps.begin(); it2 != executedOps.end(); it2++) {
      opcode_t op = it2->first;
      const inst_name_t& name = InstructionMap::name(op);

      if (BATCH_MODE)
        cout << "<@SUBTABLE>operations!op_name:" << name << "!exec_count:" << executedOps[op] << "</@SUBTABLE>" << endl;

      cout << "    ";
      cout.width(14);

      cout << std::left << name << executedOps[op]
           << "\t(" << percentage(executedOps[op],executed) << ")\n";
    }
  }
}

void Operations::dumpEventCounts(std::ostream& os) {
  os << "# ALU\n";

  // Special case for the ALU operations
  CounterMap<function_t>::iterator it;
  for(it = executedFns.begin(); it != executedFns.end(); it++) {
    function_t fn = it->first;
    const inst_name_t& name = InstructionMap::name((opcode_t)0, fn);
    os << name << "\t" << executedFns[fn] << "\n";
  }

  // Non-ALU operations
  CounterMap<opcode_t>::iterator it2;
  for(it2 = executedOps.begin(); it2 != executedOps.end(); it2++) {
    opcode_t op = it2->first;
    const inst_name_t& name = InstructionMap::name(op);
    os << name << "\t" << executedOps[op] << "\n";
  }

  os << "hd_in1\t" << hdIn1 << "\n"
        "hd_in2\t" << hdIn2 << "\n"
        "hd_out\t" << hdOut << "\n"
        "same_op\t" << sameOp << "\n"
        "\n";
}
