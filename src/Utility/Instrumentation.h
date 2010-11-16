/*
 * Instrumentation.h
 *
 * Record statistics on runtime behaviour, so they can be collated and
 * summarised when execution completes.
 *
 *  Created on: 16 Jun 2010
 *      Author: db434
 */

#ifndef INSTRUMENTATION_H_
#define INSTRUMENTATION_H_

#include "../Typedefs.h"

class DecodedInst;

namespace Instrumentation {

//public:

  // Record whether there was a cache hit or miss.
  void IPKCacheHit(bool hit);

  // Record that memory was read from.
  void memoryRead();

  // Record that memory was written to.
  void memoryWrite();

  // Record that a particular core stalled or unstalled.
  void stalled(ComponentID id, bool stalled);

  // Record that a particular core became idle or active.
  void idle(ComponentID id, bool idle);

  // End execution immediately.
  void endExecution();

  // Record that data was sent over the network from a start point to an end
  // point.
  void networkTraffic(ChannelID startID, ChannelID endID);

  // Record that data was sent over a particular link of the network.
  void networkActivity(ChannelIndex source, ChannelIndex destination, //bool isData?
                       double distance, int bitsSwitched);

  // Record whether a particular operation was executed or not.
  void operation(DecodedInst inst, bool executed, ComponentID id);

  // Print the results of instrumentation.
  void printStats();

//private:

  // Return the current clock cycle count.
  int currentCycle();

}

#endif /* INSTRUMENTATION_H_ */
