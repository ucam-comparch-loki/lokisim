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

  static void reset();

  // Record communication patterns.
  static void traffic(const ComponentID& startID, const ComponentID& endID);

  // Record switching activity for different types of network.
  static void crossbarInput(const NetworkData& oldData, const NetworkData& newData,
                            const PortIndex input);
  static void crossbarOutput(const NetworkData& oldData, const NetworkData& newData);
  static void multicastTraffic(const NetworkData& oldData, const NetworkData& newData,
                               const PortIndex input);
  static void globalTraffic(const NetworkData& oldData, const NetworkData& newData);

  // Record that 1 unit of bandwidth was used by a module (using its name).
  // Assumed that each module has 1 unit per cycle available.
  static void recordBandwidth(const char* name);

  // An arbiter performed a computation.
  static void arbitration();
  static void arbiterCreated();

  static void printStats();
  static void printSummary();
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

  // Record the bandwidth used by various modules. Could automatically record
  // data for every signal, but there might be uninteresting signals in there.
  static CounterMap<const char*> bandwidth;

};

}

#endif /* NETWORK_INSTRUMENTATION_H_ */
