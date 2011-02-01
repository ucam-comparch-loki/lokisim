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

class DecodedInst;

namespace Instrumentation {

  // Record whether there was a cache hit or miss when a fetch occurred.
  void l0TagCheck(ComponentID core, bool hit);

  // Instruction packet cache was read from.
  void l0Read(ComponentID core);

  // Instruction packet cache was written to.
  void l0Write(ComponentID core);

  // The decoder consumes a significant amount of energy, and there are a few
  // techniques to reduce its activity, so record how active it is.
  void decoded(ComponentID core, const DecodedInst& dec);

  // The desired data ended up being forwarded, rather than read from a
  // register. This provides optimisation opportunities.
  void dataForwarded(ComponentID core, RegisterIndex reg);

  // Register was read from. Collect indirect information too?
  void registerRead(ComponentID core, RegisterIndex reg);

  // A register was written to.
  void registerWrite(ComponentID core, RegisterIndex reg);

  // Record that a stall (pipeline) register was used.
  void stallRegUse(ComponentID core);

  // Record that memory performed a tag check.
  void l1TagCheck(MemoryAddr address, bool hit);

  // Record that memory was read from.
  void l1Read(MemoryAddr address, bool isInstruction);

  // Record that memory was written to.
  void l1Write(MemoryAddr address);

  // Record that a particular core stalled or unstalled.
  void stalled(ComponentID id, bool stalled, int reason=0);

  // Record that a particular core became idle or active.
  void idle(ComponentID id, bool idle);

  // End execution immediately.
  void endExecution();

  // Record that data was sent over the network from a start point to an end
  // point.
  void networkTraffic(ChannelID startID, ChannelID endID);

  // Record that data was sent over a particular link of the network.
  void networkActivity(ComponentID network, ChannelIndex source,
                       ChannelIndex destination, double distance, int bitsSwitched);

  // Record whether a particular operation was executed or not.
  void operation(ComponentID id, const DecodedInst& inst, bool executed);

  // Return the current clock cycle count.
  int currentCycle();

  // Load a library to allow power consumption estimates later.
  void loadPowerLibrary(const std::string& filename);

}

#endif /* INSTRUMENTATION_H_ */
