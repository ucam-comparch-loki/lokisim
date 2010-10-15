/*
 * InterclusterNetwork.h
 *
 * The network found within an individual tile, connecting together all of
 * the clusters and memories.
 *
 * A network contains two sub-networks:
 *   Data
 *   Credits
 *
 * When a piece of data is to be sent across the network:
 *   1. The data is sent.
 *   2. A credit is sent back when the data is consumed by the target.
 *
 * Before sending any data to a port, a port claim must be made so the target
 * knows where to send credits back to. This consists of an AddressedWord
 * holding an identifier for the sending port, and with the portClaim field
 * set to true.
 *
 *  Created on: 15 Jan 2010
 *      Author: db434
 */

#ifndef INTERCLUSTERNETWORK_H_
#define INTERCLUSTERNETWORK_H_

#include "Interconnect.h"
#include "RoutingSchemes/RoutingScheme.h"

typedef sc_in<AddressedWord>  input_port;
typedef sc_out<AddressedWord> output_port;

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

  // Responses from each input of each component.
  // NUM_CLUSTER_INPUTS * COMPONENTS_PER_TILE
  input_port  *creditsIn;

  // Responses sent to each output of each component.
  // NUM_CLUSTER_OUTPUTS * COMPONENTS_PER_TILE
  output_port *creditsOut;

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
  virtual void routeCredits();
  virtual void routeData();

  // The routing technique will be the same for all three sub-networks.
  // We have to do something extra if dealing with requests, so a boolean is
  // included to show that.
  virtual void route(input_port inputs[], output_port outputs[], int length,
                     std::vector<bool>& sent, bool instrumentation = false);

//==============================//
// Local state
//==============================//

protected:

  // The number of input and output ports required by this network.
  static const int  numInputs, numOutputs;

  // A bit for each output, showing whether or not information has already been
  // written to it, allowing us to avoid writing to the same output twice.
  std::vector<bool> sentCredits, sentData;

};

#endif /* INTERCLUSTERNETWORK_H_ */
