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

  static void traffic(ComponentID startID, ComponentID endID);
  static void printStats();

private:
  static CounterMap<int> commDistances;
  static CounterMap<ComponentID> producers;
  static CounterMap<ComponentID> consumers;
  static int maxDistance;
  static double totalDistance;

};

}

#endif /* NETWORK_H_ */
