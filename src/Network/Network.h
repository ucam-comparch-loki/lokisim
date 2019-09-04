/*
 * Network.h
 *
 * Base class for all networks.
 *
 *  Created on: 19 Mar 2019
 *      Author: db434
 */

#ifndef SRC_NETWORK_NETWORK_H_
#define SRC_NETWORK_NETWORK_H_

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

  // Some extra initialisation which is performed after all ports have been
  // bound.
  virtual void end_of_elaboration();

  // Send data on each output port on clock edges.
  virtual void sendData(PortIndex output);

  // Wrapper for inputs[input].read(). This version is multicast-safe, and only
  // consumes the data when it has been read the correct number of times.
  virtual Flit<T> readData(PortIndex input);

  // Interpret data on input ports and determine which output will be used.
  virtual void newData(PortIndex input);

  virtual void updateRequests(PortIndex input);

//============================================================================//
// Local state
//============================================================================//

private:

  // For each output port, record which inputs are waiting to send data to it.
  vector<request_list_t> requests;

  vector<RoundRobinArbiter> arbiters;

  // For multicast, track how many destinations have received the data so far.
  // Only consume an input when the counter reaches zero.
  vector<uint> copiesRemaining;

};

#endif /* SRC_NETWORK_NETWORK_H_ */
