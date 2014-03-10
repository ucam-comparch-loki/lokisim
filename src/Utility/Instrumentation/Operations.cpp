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

using namespace Instrumentation;

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

CounterMap<ComponentID> Operations::numOps_;
count_t Operations::numDecodes_ = 0;

void Operations::init() {
  lastIn1 = new int32_t[NUM_CORES];
  lastIn2 = new int32_t[NUM_CORES];
  lastOut = new int32_t[NUM_CORES];
  lastFn  = new function_t[NUM_CORES];
}

void Operations::end() {
  delete[] lastIn1; lastIn1 = NULL;
  delete[] lastIn2; lastIn2 = NULL;
  delete[] lastOut; lastOut = NULL;
  delete[] lastFn;  lastFn  = NULL;
}

void Operations::decoded(const ComponentID& core, const DecodedInst& dec) {
  // May later care about the operation, since different ones require different
  // decode energies?
  numDecodes_++;
}

void Operations::executed(const ComponentID& core, const DecodedInst& dec, bool executed) {
  // Always increase numOps - this is used to determine if we're making progress.
  numOps_.increment(core);

  // Want to keep track of the number of operations so we can tell if we're
  // making progress, but only want the rest of the data when we ask for it.
  if (!ENERGY_TRACE)
    return;

  if (!executed) {
    unexecuted++;
    return;
  }

  // Basic logging for "ordinary" operations. Extra logging for ALU operations.
  if (dec.opcode() > 1)
    executedOps.increment(dec.opcode());
  else
    executedFns.increment(dec.function());

  if (ENERGY_TRACE && dec.isALUOperation()) {
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
count_t Operations::numOperations()            {return numOps_.numEvents();}

count_t Operations::numOperations(const ComponentID& core) {
  return numOps_[core];
}

count_t Operations::numOperations(opcode_t op, function_t function) {
  // Special case for ALU operations.
  if (op <= 1)
    return executedFns[function];
  else
    return executedOps[op];
}

void Operations::printStats() {
  if (BATCH_MODE)
	cout << "<@GLOBAL>operation_count:" << numOps_.numEvents() << "</@GLOBAL>" << endl;

  if(numOps_.numEvents() > 0) {
    cout << "Operations:\n" <<
      "  Total: " << numOps_.numEvents() << "\n" <<
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
  // Stores take two cycles to decode, so the decoder is active for longer.
  count_t decodeCycles = numDecodes_ + executedOps[InstructionMap::OP_STB]
                                     + executedOps[InstructionMap::OP_STHW]
                                     + executedOps[InstructionMap::OP_STW];

  os << xmlBegin("decoder")             << "\n"
     << xmlNode("instances", NUM_CORES) << "\n"
     << xmlNode("active", decodeCycles) << "\n"
     << xmlNode("decodes", numDecodes_) << "\n"
     << xmlEnd("decoder")               << "\n";

  os << xmlBegin("alu")                 << "\n"
     << xmlNode("instances", NUM_CORES) << "\n";

  // Special case for the ALU operations
  CounterMap<function_t>::iterator it;
  for(it = executedFns.begin(); it != executedFns.end(); it++) {
    function_t fn = it->first;
    const inst_name_t& name = InstructionMap::name((opcode_t)0, fn);
    os << xmlNode(name.c_str(), executedFns[fn]) << "\n";
  }

  // Non-ALU operations
  CounterMap<opcode_t>::iterator it2;
  for(it2 = executedOps.begin(); it2 != executedOps.end(); it2++) {
    opcode_t op = it2->first;
    const inst_name_t& name = InstructionMap::name(op);
    os << xmlNode(name.c_str(), executedOps[op]) << "\n";
  }

  // Initial modelling suggests that a couple of operations consume
  // significantly more energy than the others.
  count_t highEnergy = executedFns[InstructionMap::FN_SETGTE]
                     + executedOps[InstructionMap::OP_SETGTEI]
                     + executedOps[InstructionMap::OP_SETGTEI_P]
                     + executedFns[InstructionMap::FN_ADDU]
                     + executedOps[InstructionMap::OP_ADDUI]
                     + executedOps[InstructionMap::OP_ADDUI_P];

  // Record multiplier activity separately from ALU activity, and remove non-ALU
  // operations from the total count.
  count_t totalOps   = executedOps.numEvents() + executedFns.numEvents()
                     - executedOps[InstructionMap::OP_MULHW]
                     - executedOps[InstructionMap::OP_MULHWU]
                     - executedOps[InstructionMap::OP_MULLW]
                     - executedOps[InstructionMap::OP_TSTCH]
                     - executedOps[InstructionMap::OP_TSTCHI]
                     - executedOps[InstructionMap::OP_TSTCH_P]
                     - executedOps[InstructionMap::OP_TSTCHI_P]
                     - executedOps[InstructionMap::OP_SELCH]
                     - executedOps[InstructionMap::OP_IBJMP]
                     - executedOps[InstructionMap::OP_RMTEXECUTE]
                     - executedOps[InstructionMap::OP_RMTNXIPK];

  os << xmlNode("hd_in1", hdIn1)            << "\n"
     << xmlNode("hd_in2", hdIn2)            << "\n"
     << xmlNode("hd_out", hdOut)            << "\n"
     << xmlNode("same_op", sameOp)          << "\n"
     << xmlNode("total_ops", totalOps)      << "\n"
     << xmlNode("active", totalOps)         << "\n"
     << xmlNode("high_energy", highEnergy)  << "\n"
     << xmlEnd("alu")                       << "\n";

  count_t multiplies = executedOps[InstructionMap::OP_MULHW]
                     + executedOps[InstructionMap::OP_MULHWU]
                     + executedOps[InstructionMap::OP_MULLW];
  count_t highWord   = executedOps[InstructionMap::OP_MULHW]
                     + executedOps[InstructionMap::OP_MULHWU];

  os << xmlBegin("multiplier")          << "\n"
     << xmlNode("instances", NUM_CORES) << "\n"
     << xmlNode("active", multiplies)   << "\n"
     << xmlNode("high_word", highWord)  << "\n"
     << xmlEnd("multiplier")            << "\n";

  // All operations, including non-ALU ones.
  os << "\n"
     << xmlNode("operations", executedOps.numEvents() + executedFns.numEvents())
     << "\n";
}
