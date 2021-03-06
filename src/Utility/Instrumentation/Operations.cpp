/*
 * Operations.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include "Operations.h"
#include "../Instrumentation.h"
#include "../Trace/Callgrind.h"
#include "../../Datatype/Identifier.h"
#include "../../Datatype/DecodedInst.h"
#include "../../Tile/Core/Core.h"
#include "../ISA.h"

using namespace Instrumentation;

// A separate counter for determining progress. This one always gets
// incremented, but the others turn on and off depending on if data is being
// collected.
count_t totalInstructions = 0;

CounterMap<opcode_t> Operations::executedOps;
CounterMap<function_t> Operations::executedFns;
count_t Operations::unexecuted;

vector<int32_t> Operations::lastIn1;
vector<int32_t> Operations::lastIn2;
vector<int32_t> Operations::lastOut;
vector<function_t> Operations::lastFn;

count_t Operations::hdIn1 = 0;
count_t Operations::hdIn2 = 0;
count_t Operations::hdOut = 0;
count_t Operations::sameOp = 0;

CounterMap<ComponentID> Operations::numOps_;
count_t Operations::numDecodes_ = 0;
CounterMap<ComponentID> Operations::numMemLoads;
CounterMap<ComponentID> Operations::numMergedMemLoads;
CounterMap<ComponentID> Operations::numMemStores;
CounterMap<ComponentID> Operations::numChanReads;
CounterMap<ComponentID> Operations::numMergedChanReads; // i.e. packed with a useful instruction
CounterMap<ComponentID> Operations::numChanWrites;
CounterMap<ComponentID> Operations::numMergedChanWrites;
CounterMap<ComponentID> Operations::numArithOps;
CounterMap<ComponentID> Operations::numCondOps;


void Operations::init(const chip_parameters_t& params) {
  lastIn1.resize(params.totalCores());
  lastIn2.resize(params.totalCores());
  lastOut.resize(params.totalCores());
  lastFn.resize(params.totalCores());
}

void Operations::reset() {
  lastIn1.assign(lastIn1.size(), 0);
  lastIn2.assign(lastIn2.size(), 0);
  lastOut.assign(lastOut.size(), 0);
  lastFn.assign(lastFn.size(), (function_t)0);

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

void Operations::decoded(const ComponentID& core, const DecodedInst& dec) {
  if (!Instrumentation::collectingStats()) return;

  // May later care about the operation, since different ones require different
  // decode energies?
  numDecodes_++;
}

void Operations::executed(const Core& core, const DecodedInst& dec, bool executed) {

  totalInstructions++;
  CoreIndex coreID = core.globalCoreIndex();

  if (Callgrind::acceptingData())
    Callgrind::instructionExecuted(coreID, dec.location(), Instrumentation::currentCycle());
  if (!Instrumentation::collectingStats()) return;

  // Always increase numOps - this is used to determine if we're making progress.
  numOps_.increment(core.id);

  if (!executed) {
    unexecuted++;
    return;
  }

  switch(dec.opcode()) {
    case ISA::OP_LDW:
    case ISA::OP_LDHWU:
    case ISA::OP_LDBU:
      numMemLoads.increment(core.id);
      break;
 
    case ISA::OP_STW:
    case ISA::OP_STHW:
    case ISA::OP_STB:
      numMemStores.increment(core.id);
      break;

    default:
      break;
  }
 
  // Assume channel 2 is the memory channel.
  if (dec.sourceReg1() == 2 || dec.sourceReg2() == 2) {
    if (dec.function() != ISA::FN_OR || dec.sourceReg2() != 0) {
      numMergedMemLoads.increment(core.id);
    }
    else {
      numMemLoads.increment(core.id);
    }
  }

  // Assume all other channels are for core-to-core communication.
  if ((dec.sourceReg1() >= 3 && dec.sourceReg1() <= 7) ||
      (dec.sourceReg2() >= 3 && dec.sourceReg2() <= 7)) {
    numChanReads.increment(core.id);
    if (dec.function() != ISA::FN_OR || dec.sourceReg2() != 0) {
      numMergedChanReads.increment(core.id);
    }
  }

  // Assume all output channels except {0,1} are for core-to-core communication.
  if (dec.channelMapEntry() >= 2 && dec.channelMapEntry() < 15) {
    numChanWrites.increment(core.id);
    if (dec.function() != ISA::FN_OR || dec.sourceReg1() == 2 || dec.sourceReg2() == 2) {
      numMergedChanWrites.increment(core.id);
    }
  }
 
  switch (dec.opcode()) {
    case 0:
    case 1:
      if ((dec.function() < ISA::FN_SETEQ) || (dec.function() > ISA::FN_SETGTEU)) {
        numArithOps.increment(core.id);
      } else {
        numCondOps.increment(core.id);
      }
      break;
    case ISA::OP_NORI:
    case ISA::OP_NORI_P:
    case ISA::OP_ANDI:
    case ISA::OP_ANDI_P:
    case ISA::OP_ORI:
    case ISA::OP_ORI_P:
    case ISA::OP_XORI:
    case ISA::OP_XORI_P:
    case ISA::OP_SLLI:
    case ISA::OP_SRLI:
    case ISA::OP_SRLI_P:
    case ISA::OP_SRAI:
    case ISA::OP_ADDUI:
    case ISA::OP_ADDUI_P:
    case ISA::OP_MULHW:
    case ISA::OP_MULLW:
    case ISA::OP_MULHWU:
      numArithOps.increment(core.id);
      break;

    case ISA::OP_SETEQI:
    case ISA::OP_SETEQI_P:
    case ISA::OP_SETNEI:
    case ISA::OP_SETNEI_P:
    case ISA::OP_SETLTI:
    case ISA::OP_SETLTI_P:
    case ISA::OP_SETLTUI:
    case ISA::OP_SETLTUI_P:
    case ISA::OP_SETGTEI:
    case ISA::OP_SETGTEI_P:
    case ISA::OP_SETGTEUI:
    case ISA::OP_SETGTEUI_P:
    case ISA::OP_PSEL:
    case ISA::OP_PSEL_FETCH:
    case ISA::OP_PSEL_FETCHR:
      numCondOps.increment(core.id);
      break;

    default:
      break;
  }

  // Want to keep track of the number of operations so we can tell if we're
  // making progress, but only want the rest of the data when we ask for it.
  if (!ENERGY_TRACE)
    return;

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

count_t Operations::allOperations()            {return totalInstructions;}

void Operations::printStats() {

  if(numOps_.numEvents() > 0) {
    cout << "Operations:\n" <<
      "  Total: " << numOps_.numEvents() << "\n" <<
      "  Operations executed:\n";

    int executed = executedOps.numEvents() + executedFns.numEvents();

    CounterMap<function_t>::iterator it;
    for(it = executedFns.begin(); it != executedFns.end(); it++) {
      function_t fn = it->first;
      const inst_name_t& name = ISA::name((opcode_t)0, fn);

      cout << "    ";
      cout.width(14);

      cout << std::left << name << executedFns[fn]
           << "\t(" << percentage(executedFns[fn],executed) << ")\n";
    }

    CounterMap<opcode_t>::iterator it2;
    for(it2 = executedOps.begin(); it2 != executedOps.end(); it2++) {
      opcode_t op = it2->first;
      const inst_name_t& name = ISA::name(op);

      cout << "    ";
      cout.width(14);

      cout << std::left << name << executedOps[op]
           << "\t(" << percentage(executedOps[op],executed) << ")\n";
    }
  }
}

void Operations::printSummary(const chip_parameters_t& params) {
  using std::clog;

  clog << "Average IPC: ";
  clog << std::fixed;
  clog.precision(2);
  clog << ((double)numOperations() / (double)Instrumentation::cyclesStatsCollected()) << endl;
}

void Operations::dumpEventCounts(std::ostream& os, const chip_parameters_t& params) {
  // Stores take two cycles to decode, so the decoder is active for longer.
  count_t decodeCycles = numDecodes_ + executedOps[ISA::OP_STB]
                                     + executedOps[ISA::OP_STHW]
                                     + executedOps[ISA::OP_STW];

  os << xmlBegin("decoder")             << "\n"
     << xmlNode("instances", params.totalCores()) << "\n"
     << xmlNode("active", decodeCycles) << "\n"
     << xmlNode("decodes", numDecodes_) << "\n"
     << xmlEnd("decoder")               << "\n";

  os << xmlBegin("alu")                 << "\n"
     << xmlNode("instances", params.totalCores()) << "\n";

  // Special case for the ALU operations
  CounterMap<function_t>::iterator it;
  for(it = executedFns.begin(); it != executedFns.end(); it++) {
    function_t fn = it->first;
    const inst_name_t& name = ISA::name((opcode_t)0, fn);
    os << xmlNode(name.c_str(), executedFns[fn]) << "\n";
  }

  // Non-ALU operations
  CounterMap<opcode_t>::iterator it2;
  for(it2 = executedOps.begin(); it2 != executedOps.end(); it2++) {
    opcode_t op = it2->first;
    const inst_name_t& name = ISA::name(op);
    os << xmlNode(name.c_str(), executedOps[op]) << "\n";
  }

  // Initial modelling suggests that a couple of operations consume
  // significantly more energy than the others.
  count_t highEnergy = executedFns[ISA::FN_SETGTE]
                     + executedOps[ISA::OP_SETGTEI]
                     + executedOps[ISA::OP_SETGTEI_P]
                     + executedFns[ISA::FN_ADDU]
                     + executedOps[ISA::OP_ADDUI]
                     + executedOps[ISA::OP_ADDUI_P];

  // Record multiplier activity separately from ALU activity, and remove non-ALU
  // operations from the total count.
  count_t totalOps   = executedOps.numEvents() + executedFns.numEvents()
                     - executedOps[ISA::OP_MULHW]
                     - executedOps[ISA::OP_MULHWU]
                     - executedOps[ISA::OP_MULLW]
                     - executedOps[ISA::OP_TSTCHI]
                     - executedOps[ISA::OP_TSTCHI_P]
                     - executedOps[ISA::OP_SELCH]
                     - executedOps[ISA::OP_IBJMP]
                     - executedOps[ISA::OP_RMTEXECUTE]
                     - executedOps[ISA::OP_RMTNXIPK];

  os << xmlNode("hd_in1", hdIn1)            << "\n"
     << xmlNode("hd_in2", hdIn2)            << "\n"
     << xmlNode("hd_out", hdOut)            << "\n"
     << xmlNode("same_op", sameOp)          << "\n"
     << xmlNode("total_ops", totalOps)      << "\n"
     << xmlNode("active", totalOps)         << "\n"
     << xmlNode("high_energy", highEnergy)  << "\n"
     << xmlEnd("alu")                       << "\n";

  count_t multiplies = executedOps[ISA::OP_MULHW]
                     + executedOps[ISA::OP_MULHWU]
                     + executedOps[ISA::OP_MULLW];
  count_t highWord   = executedOps[ISA::OP_MULHW]
                     + executedOps[ISA::OP_MULHWU];

  os << xmlBegin("multiplier")          << "\n"
     << xmlNode("instances", params.totalCores()) << "\n"
     << xmlNode("active", multiplies)   << "\n"
     << xmlNode("high_word", highWord)  << "\n"
     << xmlEnd("multiplier")            << "\n";

  // All operations, including non-ALU ones.
  os << "\n"
     << xmlNode("operations", executedOps.numEvents() + executedFns.numEvents())
     << "\n";
}
