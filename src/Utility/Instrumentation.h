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

#include <string>
#include "../Typedefs.h"
#include "../Datatype/ChannelID.h"

class DecodedInst;

namespace Instrumentation {

  // Record whether there was a cache hit or miss when a fetch occurred.
  void l0TagCheck(const ComponentID& core, bool hit);

  // Instruction packet cache was read from.
  void l0Read(const ComponentID& core);

  // Instruction packet cache was written to.
  void l0Write(const ComponentID& core);

  // The decoder consumes a significant amount of energy, and there are a few
  // techniques to reduce its activity, so record how active it is.
  void decoded(const ComponentID& core, const DecodedInst& dec);

  // The desired data ended up being forwarded, rather than read from a
  // register. This provides optimisation opportunities.
  void dataForwarded(const ComponentID& core, RegisterIndex reg);

  // Register was read from. Collect indirect information too?
  void registerRead(const ComponentID& core, RegisterIndex reg);

  // A register was written to.
  void registerWrite(const ComponentID& core, RegisterIndex reg);

  // Record that a stall (pipeline) register was used.
  void stallRegUse(const ComponentID& core);

  // Record that memory performed a tag check.
  void l1TagCheck(MemoryAddr address, bool hit);

  // Record that memory was read from.
  void l1Read(MemoryAddr address, bool isInstruction);

  // Record that memory was written to.
  void l1Write(MemoryAddr address);

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

  // Record that a particular core stalled or unstalled.
  void stalled(const ComponentID& id, bool stalled, int reason=0);

  // Record that a particular core became idle or active.
  void idle(const ComponentID& id, bool idle);

  // End execution immediately.
  void endExecution();

  // Returns whether execution has finished.
  bool executionFinished();

  // Record that data was sent over the network from a start point to an end
  // point.
  void networkTraffic(const ChannelID& startID, const ChannelID& endID);

  // Record that data was sent over a particular link of the network.
  void networkActivity(const ComponentID& network, ChannelIndex source,
                       ChannelIndex destination, double distance, int bitsSwitched);

  // Record whether a particular operation was executed or not.
  void executed(const ComponentID& id, const DecodedInst& inst, bool executed);

  // Return the current clock cycle count.
  unsigned long currentCycle();

  // Load a library to allow power consumption estimates later.
  void loadPowerLibrary(const std::string& filename);

}

#endif /* INSTRUMENTATION_H_ */
