/*
 * Network.h
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#ifndef NETWORK_INSTRUMENTATION_H_
#define NETWORK_INSTRUMENTATION_H_

#include "../../Datatype/ComponentID.h"
#include "InstrumentationBase.h"
#include "CounterMap.h"

namespace Instrumentation {

class Network: public InstrumentationBase {

public:

  // Tells who is communicating with whom.
  static void traffic(const ComponentID& startID, const ComponentID& endID);

  static void printStats();

private:

  static CounterMap<ComponentID> producers;
  static CounterMap<ComponentID> consumers;

};

}

#endif /* NETWORK_INSTRUMENTATION_H_ */
