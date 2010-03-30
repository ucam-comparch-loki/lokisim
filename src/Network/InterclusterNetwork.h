/*
 * InterclusterNetwork.h
 *
 *  Created on: 15 Jan 2010
 *      Author: db434
 */

#ifndef INTERCLUSTERNETWORK_H_
#define INTERCLUSTERNETWORK_H_

#include "Interconnect.h"
#include "Routing Schemes/RoutingScheme.h"

typedef sc_in<AddressedWord> input_port;
typedef sc_out<Word>         output_port;

class InterclusterNetwork: public Interconnect {

//==============================//
// Ports
//==============================//

public:

  // Data sent from each output of each component.
  // NUM_CLUSTER_OUTPUTS * COMPONENTS_PER_TILE
  input_port  *dataIn;

  // Data sent to each input of each component.
  // NUM_CLUSTER_INPUTS * COMPONENTS_PER_TILE
  output_port *dataOut;

  // Requests from each output of each component.
  // NUM_CLUSTER_OUTPUTS * COMPONENTS_PER_TILE
  input_port  *requestsIn;

  // Responses from each input of each component.
  // NUM_CLUSTER_INPUTS * COMPONENTS_PER_TILE
  input_port  *responsesIn;

  // Requests sent to each input of each component.
  // NUM_CLUSTER_INPUTS * COMPONENTS_PER_TILE
  output_port *requestsOut;

  // Responses sent to each output of each component.
  // NUM_CLUSTER_OUTPUTS * COMPONENTS_PER_TILE
  output_port *responsesOut;

  // Some AddressedWord outputs for longer communications?

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(InterclusterNetwork);
  InterclusterNetwork(sc_module_name name);
  virtual ~InterclusterNetwork();

//==============================//
// Methods
//==============================//

protected:

  virtual void routeRequests();
  virtual void routeResponses();
  virtual void routeData();
  virtual void route(input_port *inputs, output_port *outputs,
                     int length, std::vector<bool>& sent);

//==============================//
// Local state
//==============================//

protected:

  // The number of input and output ports required by this network.
  static const int  numInputs, numOutputs;

  // The component responsible for the routing behaviour.
  RoutingScheme*    router;

  // A bit for each output, showing whether or not information has already been
  // written to it, allowing us to avoid writing to the same output twice.
  std::vector<bool> sentRequests, sentResponses, sentData;

};

#endif /* INTERCLUSTERNETWORK_H_ */
