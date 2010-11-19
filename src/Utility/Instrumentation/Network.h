/*
 * Network.h
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "InstrumentationBase.h"
#include "CounterMap.h"

namespace Instrumentation {

class Network: public InstrumentationBase {

public:

  // Tells who is communicating with who.
  static void traffic(ComponentID startID, ComponentID endID);

  // Tells how the communication is happening, to help determine bottlenecks
  // and power consumption.
  // Instead of ComponentID, ask for pointer to component, so we can ask it
  // as many questions as we like?
  static void activity(ComponentID network, ChannelIndex source,
      ChannelIndex destination, double distance, int bitsSwitched);

  static void printStats();

private:

  // Keep track of the distribution of communication distances. Note that in
  // order to "bin" the reported distances, they have all been multiplied by a
  // constant factor, so this will need to be undone when reading commDistances.
  static CounterMap<int> commDistances;

  static CounterMap<ComponentID> producers;
  static CounterMap<ComponentID> consumers;

  // Keep track of the maximum communication distance so we know when to stop
  // iterating through the map. TODO: sort + iterator.
  static int maxDistance;

  static double totalDistance;

  // Take into account the distance signals travel, as well as the number of
  // bits which changed.
  static double totalBitDistance;

};

}

#endif /* NETWORK_H_ */
