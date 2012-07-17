/*
 * PipelineReg.cpp
 *
 *  Created on: 3 Jul 2012
 *      Author: db434
 */

#include "PipelineReg.h"

using namespace Instrumentation;

CounterMap<size_t> PipelineReg::active;
CounterMap<size_t> PipelineReg::hammingDist;

void PipelineReg::activity(const DecodedInst& oldVal,
                           const DecodedInst& newVal,
                           PipelineRegister::PipelinePosition stage) {

  size_t width;
  int hamming = 0;

  // TODO: could look at effects of clock gating parts of the pipeline registers
  // which aren't needed.

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
//      width = 84;
      width = 96; // round up to something we have a model for
      hamming += __builtin_popcount(oldVal.operand1()  ^ newVal.operand1() );
      hamming += __builtin_popcount(oldVal.operand2()  ^ newVal.operand2() );
      hamming += __builtin_popcount(oldVal.opcode()    ^ newVal.opcode()   );
      hamming += __builtin_popcount(oldVal.function()  ^ newVal.function() );
      hamming += __builtin_popcount(oldVal.channelMapEntry() ^ newVal.channelMapEntry());
      hamming += __builtin_popcount(oldVal.predicate() ^ newVal.predicate());

      break;

    // Information sent from execute to write:
    //  * Result (32 bits)
    //  * Destination register (5 bits)
    //  * Destination channel (20 bits?)
    //  * Various other flags (3 bits?)
    case PipelineRegister::EXECUTE_WRITE:
//      width = 60;
      width = 64; // round up to something we have a model for
      hamming += __builtin_popcount(oldVal.result() ^ newVal.result());
      hamming += __builtin_popcount(oldVal.destination() ^ newVal.destination());
      hamming += __builtin_popcount(oldVal.networkDestination().toInt() ^ newVal.networkDestination().toInt());

      break;

    default:
      assert(false);
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

    os << "<pipeline_register width=\"" << width << "\">\n"
       << xmlNode("instances", NUM_CORES) << "\n"
       << xmlNode("active", activeCycles) << "\n"
       << xmlNode("write", activeCycles) << "\n"
       << xmlNode("hd", hamming) << "\n"
       << xmlEnd("pipeline_register") << "\n";
  }
}
