/*
 * Network.h
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#ifndef NETWORK_INSTRUMENTATION_H_
#define NETWORK_INSTRUMENTATION_H_

#include "InstrumentationBase.h"
#include "CounterMap.h"
#include "../../Network/NetworkTypedefs.h"

class ComponentID;

namespace Instrumentation {

class Network: public InstrumentationBase {

public:

  // Record communication patterns.
  static void traffic(const ComponentID& startID, const ComponentID& endID);

  // Record switching activity for different types of network.
  static void crossbarInput(const NetworkData& oldData, const NetworkData& newData,
                            const PortIndex input);
  static void crossbarOutput(const NetworkData& oldData, const NetworkData& newData);
  static void multicastTraffic(const NetworkData& oldData, const NetworkData& newData,
                               const PortIndex input);
  static void globalTraffic(const NetworkData& oldData, const NetworkData& newData);

  // An arbiter performed a computation.
  static void arbitration();
  static void arbiterCreated();

  static void printStats();
  static void dumpEventCounts(std::ostream& os);

private:

  static CounterMap<ComponentIndex> producers;
  static CounterMap<ComponentIndex> consumers;

  static count_t arbitrations;
  static count_t arbiters;

  // Hamming distances for the different networks.
  // In/Out = record separate Hamming distances for input and output ports.
  // DistHD = Hamming distance * physical distance (input port index - output port index)
  // Repeat = number of "intelligent repeaters" the information passed through.
  static count_t xbarInHD, xbarOutHD, xbarDistHD;
  static count_t mcastHD, mcastRepeatHD;
  static count_t globalHD;

};

}

#endif /* NETWORK_INSTRUMENTATION_H_ */
