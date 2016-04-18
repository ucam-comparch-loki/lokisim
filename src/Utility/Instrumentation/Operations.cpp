/*
 * Operations.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include "Operations.h"
#include "../InstructionMap.h"
#include "../Instrumentation.h"
#include "../Trace/Callgrind.h"
#include "../../Datatype/Identifier.h"
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

CounterMap<CoreIndex> Operations::numOps_;
count_t Operations::numDecodes_ = 0;
CounterMap<CoreIndex> Operations::numMemLoads;
CounterMap<CoreIndex> Operations::numMergedMemLoads;
CounterMap<CoreIndex> Operations::numMemStores;
CounterMap<CoreIndex> Operations::numChanReads;
CounterMap<CoreIndex> Operations::numMergedChanReads; // i.e. packed with a useful instruction
CounterMap<CoreIndex> Operations::numChanWrites;
CounterMap<CoreIndex> Operations::numMergedChanWrites;
CounterMap<CoreIndex> Operations::numArithOps;
CounterMap<CoreIndex> Operations::numCondOps;


void Operations::init() {
  lastIn1 = new int32_t[NUM_CORES];
  lastIn2 = new int32_t[NUM_CORES];
  lastOut = new int32_t[NUM_CORES];
  lastFn  = new function_t[NUM_CORES];

  unexecuted = hdIn1 = hdIn2 = hdOut = sameOp = numDecodes_ = 0;

  executedOps.clear();
  executedFns.clear();
  numOps_.clear();
  numMemLoads.clear();
  numMergedMemLoads.clear();
  numMemStores.clear();
  numChanReads.clear();
  numMergedChanReads.clear();
  numChanWrites.clear();
  numMergedChanWrites.clear();
  numArithOps.clear();
  numCondOps.clear();
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
  CoreIndex coreID = core.globalCoreNumber();

  // Always increase numOps - this is used to determine if we're making progress.
  numOps_.increment(coreID);

  if (Callgrind::acceptingData())
    Callgrind::instructionExecuted(core, dec.location(), Instrumentation::currentCycle());

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

  if (ENERGY_TRACE && dec.isExecuteStageOperation()) {
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

  switch(dec.opcode()) {
    case InstructionMap::OP_LDW:
    case InstructionMap::OP_LDHWU:
    case InstructionMap::OP_LDBU:
      numMemLoads.increment(coreID);
      break;
 
    case InstructionMap::OP_STW:
    case InstructionMap::OP_STHW:
    case InstructionMap::OP_STB:
      numMemStores.increment(coreID);
      break;

    default:
      break;
  }
 
  if (dec.sourceReg1() == 2 || dec.sourceReg2() == 2) {
    numMemLoads.increment(coreID);
    if (dec.function() != InstructionMap::FN_OR || dec.sourceReg2() != 0) {
      numMergedMemLoads.increment(coreID);
    }
  }

  if (dec.sourceReg1() == 3 || dec.sourceReg2() == 3 || dec.sourceReg1() == 4 || dec.sourceReg2() == 4) {
    numChanReads.increment(coreID);
    if (dec.function() != InstructionMap::FN_OR || dec.sourceReg2() != 0) {
      numMergedChanReads.increment(coreID);
    }
  }

  if (dec.channelMapEntry() == 2 || dec.channelMapEntry() == 3) {
    numChanWrites.increment(coreID);
    if (dec.function() != InstructionMap::FN_OR || dec.sourceReg1() == 2 || dec.sourceReg2() == 2) {
      numMergedChanWrites.increment(coreID);
    }
  }
 
  switch (dec.opcode()) {
    case 0:
    case 1:
      if ((dec.function() < InstructionMap::FN_SETEQ) || (dec.function() > InstructionMap::FN_SETGTEU)) {
        numArithOps.increment(coreID);
      } else {
        numCondOps.increment(coreID);
      }
      break;
    case InstructionMap::OP_NORI:
    case InstructionMap::OP_NORI_P:
    case InstructionMap::OP_ANDI:
    case InstructionMap::OP_ANDI_P:
    case InstructionMap::OP_ORI:
    case InstructionMap::OP_ORI_P:
    case InstructionMap::OP_XORI:
    case InstructionMap::OP_XORI_P:
    case InstructionMap::OP_SLLI:
    case InstructionMap::OP_SRLI:
    case InstructionMap::OP_SRLI_P:
    case InstructionMap::OP_SRAI:
    case InstructionMap::OP_ADDUI:
    case InstructionMap::OP_ADDUI_P:
    case InstructionMap::OP_MULHW:
    case InstructionMap::OP_MULLW:
    case InstructionMap::OP_MULHWU:
      numArithOps.increment(coreID);
      break;

    case InstructionMap::OP_SETEQI:
    case InstructionMap::OP_SETEQI_P:
    case InstructionMap::OP_SETNEI:
    case InstructionMap::OP_SETNEI_P:
    case InstructionMap::OP_SETLTI:
    case InstructionMap::OP_SETLTI_P:
    case InstructionMap::OP_SETLTUI:
    case InstructionMap::OP_SETLTUI_P:
    case InstructionMap::OP_SETGTEI:
    case InstructionMap::OP_SETGTEI_P:
    case InstructionMap::OP_SETGTEUI:
    case InstructionMap::OP_SETGTEUI_P:
    case InstructionMap::OP_PSEL:
    case InstructionMap::OP_PSEL_FETCH:
    case InstructionMap::OP_PSEL_FETCHR:
      numCondOps.increment(coreID);
      break;

    default:
      break;
  }
}

count_t Operations::numDecodes()               {return numDecodes_;}
count_t Operations::numOperations()            {return numOps_.numEvents();}

count_t Operations::numOperations(const ComponentID& core) {
  return numOps_[core.globalCoreNumber()];
}

count_t Operations::numOperations(opcode_t op, function_t function) {
  // Special case for ALU operations.
  if (op <= 1)
    return executedFns[function];
  else
    return executedOps[op];
}

void Operations::printStats() {

  if(numOps_.numEvents() > 0) {
    cout << "Operations:\n" <<
      "  Total: " << numOps_.numEvents() << "\n" <<
      "  Operations executed:\n";

    int executed = executedOps.numEvents() + executedFns.numEvents();

    CounterMap<function_t>::iterator it;
    for(it = executedFns.begin(); it != executedFns.end(); it++) {
      function_t fn = it->first;
      const inst_name_t& name = InstructionMap::name((opcode_t)0, fn);

      cout << "    ";
      cout.width(14);

      cout << std::left << name << executedFns[fn]
           << "\t(" << percentage(executedFns[fn],executed) << ")\n";
    }

    CounterMap<opcode_t>::iterator it2;
    for(it2 = executedOps.begin(); it2 != executedOps.end(); it2++) {
      opcode_t op = it2->first;
      const inst_name_t& name = InstructionMap::name(op);

      cout << "    ";
      cout.width(14);

      cout << std::left << name << executedOps[op]
           << "\t(" << percentage(executedOps[op],executed) << ")\n";
    }
  }
}

void Operations::printSummary() {
  using std::clog;

  clog << "Average IPC: ";
  clog << std::fixed;
  clog.precision(2);
  clog << ((double)numOperations() / (double)Instrumentation::currentCycle()) << endl;
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
                     - executedOps[InstructionMap::OP_TSTCHI]
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
