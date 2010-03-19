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

class InterclusterNetwork: public Interconnect {

//==============================//
// Ports
//==============================//

public:

  // Data sent from each output of each component.
  // NUM_CLUSTER_OUTPUTS * COMPONENTS_PER_TILE
  sc_in<AddressedWord> *dataIn;

  // Data sent to each input of each component.
  // NUM_CLUSTER_INPUTS * COMPONENTS_PER_TILE
  sc_out<Word>         *dataOut;

  // Requests from each output of each component.
  // NUM_CLUSTER_OUTPUTS * COMPONENTS_PER_TILE
  sc_in<AddressedWord> *requestsIn;

  // Responses from each input of each component.
  // NUM_CLUSTER_INPUTS * COMPONENTS_PER_TILE
  sc_in<AddressedWord> *responsesIn;

  // Requests sent to each input of each component.
  // NUM_CLUSTER_INPUTS * COMPONENTS_PER_TILE
  sc_out<Word>         *requestsOut;

  // Responses sent to each output of each component.
  // NUM_CLUSTER_OUTPUTS * COMPONENTS_PER_TILE
  sc_out<Word>         *responsesOut;

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
  virtual void route(sc_in<AddressedWord> *inputs, sc_out<Word> *outputs, int length);

//==============================//
// Local state
//==============================//

protected:

  int            numInputs, numOutputs;
  RoutingScheme* router;

};

#endif /* INTERCLUSTERNETWORK_H_ */
