/*
 * Network.h
 *
 *  Created on: 19 Mar 2019
 *      Author: db434
 */

#ifndef SRC_NETWORK_TOPOLOGIES_NETWORK_H_
#define SRC_NETWORK_TOPOLOGIES_NETWORK_H_

#include <set>
#include "../LokiComponent.h"
#include "Interface.h"
#include "Arbiters/RoundRobinArbiter.h"
#include "../Utility/BlockingInterface.h"
#include "../Utility/LokiVector.h"

using sc_core::sc_port;
using std::ostream;
using std::set;

template<typename T>
class Network : public LokiComponent, public BlockingInterface {

//============================================================================//
// Ports
//============================================================================//

public:
  // Data is sent through the crossbar on positive clock edges.
  // TODO: instead, include an sc_event as a constructor argument.
  ClockInput clock;

  typedef sc_port<network_source_ifc<T>> InPort;
  typedef sc_port<network_sink_ifc<T>> OutPort;
  LokiVector<InPort> inputs;
  LokiVector<OutPort> outputs;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:
  SC_HAS_PROCESS(Network);

  // TODO: add bandwidth parameter, and send multiple flits per cycle if
  // possible.
  Network(sc_module_name name, uint numInputs, uint numOutputs);

//============================================================================//
// Methods
//============================================================================//

protected:

  virtual void reportStalls(ostream& os);

  // Choose an output port to use when aiming for the given destination.
  // This must be implemented by all subclasses.
  virtual PortIndex getDestination(const ChannelID address) const = 0;
  virtual set<PortIndex> getDestinations(const ChannelID address) const;

private:

  // Some extra initialisation which is performed after all ports have been
  // bound.
  void end_of_elaboration();

  // Send data on each output port on clock edges.
  void sendData(PortIndex output);

  // Interpret data on input ports and determine which output will be used.
  void newData(PortIndex input);

  void updateRequests(PortIndex input);

//============================================================================//
// Local state
//============================================================================//

private:

  // For each output port, record which inputs are waiting to send data to it.
  vector<request_list_t> requests;

  vector<RoundRobinArbiter2> arbiters;

};

#endif /* SRC_NETWORK_TOPOLOGIES_NETWORK_H_ */
