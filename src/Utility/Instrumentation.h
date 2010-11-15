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
  void stalled(int id, bool stalled);

  // Record that a particular core became idle or active.
  void idle(int id, bool idle);

  // End execution immediately.
  void endExecution();

  // Record that data was sent over the network.
  void networkTraffic(int startID, int endID, double distance);

  // Record whether a particular operation was executed or not.
  void operation(DecodedInst inst, bool executed, int id);

  // Print the results of instrumentation.
  void printStats();

//private:

  // Return the current clock cycle count.
  int currentCycle();

}

#endif /* INSTRUMENTATION_H_ */
