/*
 * Network.h
 *
 *  Created on: 2 Nov 2010
 *      Author: db434
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "RoutingComponent.h"

class Network : public RoutingComponent {

//==============================//
// Ports
//==============================//

// Inherited from RoutingComponent:
//   clock
//   dataIn
//   dataOut
//   readyIn
//   readyOut

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Network);
  Network(sc_module_name name,
          ComponentID ID,
          ChannelID lowestID,   // Lowest channel ID accessible on this network
          ChannelID highestID,  // Highest channel ID accessible on this network
          int numInputs,        // Number of inputs this network has
          int numOutputs,       // Number of outputs this network has
          int networkType);

  virtual ~Network();

//==============================//
// Methods
//==============================//

public:

  // The input port to this network which comes from the next level of network
  // hierarchy (or off-chip).
  sc_in<AddressedWord>& externalInput() const;

  // The output port of this network which goes to the next level of network
  // hierarchy (or off-chip).
  sc_out<AddressedWord>& externalOutput() const;

  sc_in<bool>& externalReadyInput() const;
  sc_out<bool>& externalReadyOutput() const;

//==============================//
// Local state
//==============================//

protected:

  // The lowest and highest channel IDs accessible on this network. Anything
  // outside this range should be sent up to the next level of network
  // hierarchy.
  ChannelID startID, endID;

  // We may be on an upper level of the network hierarchy, where each output
  // leads to an entire subnetwork. We need to know how many channel IDs are
  // reachable through each output, so we know where to send data.
  int idsPerChannel;

  // The output to send data to if it is not destined for any component on this
  // network.
  int offNetworkOutput;

};

#endif /* NETWORK_H_ */
