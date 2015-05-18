/*
 * PipelineReg.cpp
 *
 *  Created on: 3 Jul 2012
 *      Author: db434
 */

#include "PipelineReg.h"
#include "../../Exceptions/InvalidOptionException.h"

// Toggle whether pipeline registers are heavily clock gated.
// 0 = either whole register is read/written, or none of it is.
// 1 = sections of each register can be used individually.
#define CLOCK_GATE 0

using namespace Instrumentation;

CounterMap<size_t> PipelineReg::active;
CounterMap<size_t> PipelineReg::hammingDist;

void PipelineReg::activity(const DecodedInst& oldVal,
                           const DecodedInst& newVal,
                           PipelineRegister::PipelinePosition stage) {

  size_t width;
  int hamming = 0;

  switch (stage) {

    // Information sent from fetch to decode:
    //  * encoded instruction (32 bits)
    case PipelineRegister::FETCH_DECODE:
      width = 32;
      hamming += __builtin_popcount(oldVal.toInstruction().toInt() ^ newVal.toInstruction().toInt());

      break;

    // Information sent from decode to execute:
    //  * Up to two operands (2x32 bits)
    //  * Opcode and function (11 bits)
    //  * Channel map entry (4 bits)
    //  * Various other flags (5 bits?)
    case PipelineRegister::DECODE_EXECUTE:
      width = 12;
      hamming += __builtin_popcount(oldVal.opcode()    ^ newVal.opcode()   );
      hamming += __builtin_popcount(oldVal.predicate() ^ newVal.predicate());

      if (!CLOCK_GATE || newVal.hasOperand1()) {
        hamming += __builtin_popcount(oldVal.operand1()  ^ newVal.operand1() );
        width += 32;
      }
      if (!CLOCK_GATE || newVal.hasOperand2()) {
        hamming += __builtin_popcount(oldVal.operand2()  ^ newVal.operand2() );
        width += 32;
      }
      if (!CLOCK_GATE || newVal.isExecuteStageOperation()) {
        hamming += __builtin_popcount(oldVal.function()  ^ newVal.function() );
        width += 4;
      }
      if (!CLOCK_GATE || newVal.sendsOnNetwork()) {
        hamming += __builtin_popcount(oldVal.channelMapEntry() ^ newVal.channelMapEntry());
        width += 4;
      }

      break;

    // Information sent from execute to write:
    //  * Result (32 bits)
    //  * Destination register (5 bits)
    //  * Destination channel (20 bits?)
    //  * Various other flags (3 bits?)
    case PipelineRegister::EXECUTE_WRITE:
      width = 3;

      if (!CLOCK_GATE || newVal.hasDestReg() || newVal.sendsOnNetwork()) {
        hamming += __builtin_popcount(oldVal.result() ^ newVal.result());
        width += 32;
      }
      if (!CLOCK_GATE || newVal.hasDestReg()) {
        hamming += __builtin_popcount(oldVal.destination() ^ newVal.destination());
        width += 5;
      }
      if (!CLOCK_GATE || newVal.sendsOnNetwork()) {
        hamming += __builtin_popcount(oldVal.networkDestination().flatten() ^ newVal.networkDestination().flatten());
        width += 20;
      }

      break;

    default:
      throw InvalidOptionException("pipeline register index", stage);
      break;
  }

  active.increment(width);
  hammingDist.setCount(width, hammingDist[width] + hamming);

}

void PipelineReg::dumpEventCounts(std::ostream& os) {
  CounterMap<size_t>::iterator it;

  for (it = active.begin(); it != active.end(); it++) {
    size_t width = it->first;
    count_t activeCycles = it->second;
    count_t hamming = hammingDist[width];

    os << "<pipeline_register width=\"" << width << "\">\n";

    // Only registers of these widths actually exist. All other widths are the
    // result of clock gating portions of the registers.
    if (width==32 || width==60 || width==84)
      os << xmlNode("instances", NUM_CORES) << "\n";

    os << xmlNode("active", activeCycles) << "\n"
       << xmlNode("write", activeCycles) << "\n"
       << xmlNode("hd", hamming) << "\n"
       << xmlEnd("pipeline_register") << "\n";
  }
}
