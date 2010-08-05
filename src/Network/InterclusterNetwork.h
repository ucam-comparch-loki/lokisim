/*
 * InterclusterNetwork.h
 *
 * The network found within an individual tile, connecting together all of
 * the clusters and memories.
 *
 * A network contains three sub-networks:
 *   Request
 *   Response
 *   Data
 *
 * The first two sub-networks are hidden from the programmer, and are there
 * to allow flexibility in choice of flow control mechanisms. When a piece of
 * data is to be sent across the network:
 *   1. A request is sent to see if it is safe to send.
 *   2. A response tells us whether it is safe.
 *   3. Data is sent (or back to step 1 if the request was denied).
 *
 *  Created on: 15 Jan 2010
 *      Author: db434
 */

#ifndef INTERCLUSTERNETWORK_H_
#define INTERCLUSTERNETWORK_H_

#include "Interconnect.h"
#include "RoutingSchemes/RoutingScheme.h"

typedef sc_in<AddressedWord> input_port;
typedef sc_out<Word>         output_port;

class InterclusterNetwork: public Interconnect {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>  clock;

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

  // Some AddressedWord outputs for communications to other tiles?

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

  // Route data on each network when changes are observed.
  virtual void routeRequests();
  virtual void routeResponses();
  virtual void routeData();

  // The routing technique will be the same for all three sub-networks.
  // We have to do something extra if dealing with requests, so a boolean is
  // included to show that.
  virtual void route(input_port inputs[], output_port outputs[], int length,
                     std::vector<bool>& sent, bool requests,
                     bool instrumentation = false);

//==============================//
// Local state
//==============================//

protected:

  // The number of input and output ports required by this network.
  static const int  numInputs, numOutputs;

  // A bit for each output, showing whether or not information has already been
  // written to it, allowing us to avoid writing to the same output twice.
  std::vector<bool> sentRequests, sentResponses, sentData;

  // Shows which requests weren't able to make it through the network (perhaps
  // due to a collision or the target being busy), and so should be denied.
  std::vector<bool> blockedRequests;

//==============================//
// Signals (wires)
//==============================//

protected:

  sc_signal<bool>   sendResponses;

};

#endif /* INTERCLUSTERNETWORK_H_ */
