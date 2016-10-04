/*
 * Instrumentation.h
 *
 * Record statistics on runtime behaviour, so they can be collated and
 * summarised when execution completes.
 *
 * To extract the statistics, use the Statistics class.
 *
 *  Created on: 16 Jun 2010
 *      Author: db434
 */

#ifndef INSTRUMENTATION_H_
#define INSTRUMENTATION_H_

#include "../Tile/Memory/MemoryTypedefs.h"
#include "../Typedefs.h"

class ChannelID;
class ComponentID;
class DecodedInst;

namespace Instrumentation {

  void initialise();
  void end();
  void dumpEventCounts(std::ostream& os);
  void printSummary();
  bool haveEnergyData();

  void startEventLog();
  void stopEventLog();

  // Reset all event counts to zero. Useful for ignoring a setup phase of an
  // application. Currently activated by syscall 0x27.
  void clearStats();

  // The decoder consumes a significant amount of energy, and there are a few
  // techniques to reduce its activity, so record how active it is.
  void decoded(const ComponentID& core, const DecodedInst& dec);

  // Record that background memory was read from.
  void backgroundMemoryRead(MemoryAddr address, uint32_t count);

  // Record that background memory was written to.
  void backgroundMemoryWrite(MemoryAddr address, uint32_t count);

  // Record that a particular core became idle or active.
  void idle(const ComponentID id, bool idle);

  // End execution immediately.
  void endExecution();

  // Returns whether execution has finished.
  bool executionFinished();

  // Record whether a particular operation was executed or not.
  void executed(const ComponentID& id, const DecodedInst& inst, bool executed);

  // Return the current clock cycle count.
  cycle_count_t currentCycle();

}

#endif /* INSTRUMENTATION_H_ */
