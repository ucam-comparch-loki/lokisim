/*
 * Network.h
 *
 *  Created on: 2 Nov 2010
 *      Author: db434
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "../Component.h"
#include "NetworkTypedefs.h"

class AddressedWord;

class Network : public Component {

public:

  // Options for which level in the hierarchy this network is. Different
  // networks need to look at different parts of the ChannelID type.
  //   TILE = global network (needs to see which tile to send to)
  //   COMPONENT = local network (needs to see which component to send to)
  //   CHANNEL = within a core
  //   NONE = always send to first output
  enum HierarchyLevel {TILE, COMPONENT, CHANNEL, NONE};

//==============================//
// Ports
//==============================//

public:

  ClockInput   clock;

  LokiVector<DataInput>  iData;
  LokiVector<DataOutput> oData;

//==============================//
// Constructors and destructors
//==============================//

public:

  // This would work nicely when moving down the hierarchy, but what about when
  // moving up? e.g. Credits pass through a CHANNEL network on the way to the
  // local network, but we don't want them to be routed by the channel value yet.
  Network(const sc_module_name& name,
          const ComponentID& ID,
          int numInputs,        // Number of inputs this network has
          int numOutputs,       // Number of outputs this network has
          HierarchyLevel level, // Position in the network hierarchy
          int firstOutput=0,    // The first accessible channel/component/tile
          bool externalConnection=false); // Is there a port to send data on if it
                                          // isn't for any local component?);

//==============================//
// Methods
//==============================//

public:

  // The input port to this network which comes from the next level of network
  // hierarchy (or off-chip).
  DataInput&   externalInput() const;

  // The output port of this network which goes to the next level of network
  // hierarchy (or off-chip).
  DataOutput&  externalOutput() const;

  unsigned int numInputPorts() const;
  unsigned int numOutputPorts() const;

protected:

  // Compute which output of this network will be used by the given address.
  PortIndex getDestination(const ChannelID& address) const;

//==============================//
// Local state
//==============================//

protected:

  // The channel/component/tile accessible through the first output port.
  // For example, this network may only send to memories, so whilst the target
  // component may have an ID of 8, we may want to use output port 0.
  const unsigned int firstOutput;

  const HierarchyLevel level;

  // Tells whether this network has an extra connection to handle data which
  // isn't for any local component. This extra connection will typically
  // connect to the next level of the network hierarchy.
  const bool externalConnection;

};

#endif /* NETWORK_H_ */
