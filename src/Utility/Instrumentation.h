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

#include "../Typedefs.h"

class AddressedWord;
class ChannelID;
class ComponentID;
class DecodedInst;

namespace Instrumentation {

  void initialise();
  void end();
  void dumpEventCounts(std::ostream& os);

  // The decoder consumes a significant amount of energy, and there are a few
  // techniques to reduce its activity, so record how active it is.
  void decoded(const ComponentID& core, const DecodedInst& dec);

  void memorySetMode(int bank, bool isCache, uint setCount, uint wayCount, uint lineSize);

  void memoryReadWord(int bank, MemoryAddr address, bool isMiss);
  void memoryReadHalfWord(int bank, MemoryAddr address, bool isMiss);
  void memoryReadByte(int bank, MemoryAddr address, bool isMiss);

  void memoryWriteWord(int bank, MemoryAddr address, bool isMiss);
  void memoryWriteHalfWord(int bank, MemoryAddr address, bool isMiss);
  void memoryWriteByte(int bank, MemoryAddr address, bool isMiss);

  void memoryInitiateIPKRead(int bank, bool isHandOff);
  void memoryInitiateBurstRead(int bank, bool isHandOff);
  void memoryInitiateBurstWrite(int bank, bool isHandOff);

  void memoryReadIPKWord(int bank, MemoryAddr address, bool isMiss);
  void memoryReadBurstWord(int bank, MemoryAddr address, bool isMiss);
  void memoryWriteBurstWord(int bank, MemoryAddr address, bool isMiss);

  void memoryReplaceCacheLine(int bank, bool isValid, bool isDirty);

  void memoryRingHandOff(int bank);
  void memoryRingPassThrough(int bank);

  // Record that background memory was read from.
  void backgroundMemoryRead(MemoryAddr address, uint32_t count);

  // Record that background memory was written to.
  void backgroundMemoryWrite(MemoryAddr address, uint32_t count);

  // Record that a particular core became idle or active.
  void idle(const ComponentID& id, bool idle);

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
