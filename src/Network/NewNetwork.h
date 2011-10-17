/*
 * NewNetwork.h
 *
 *  Created on: 2 Nov 2010
 *      Author: db434
 */

#ifndef NEWNETWORK_H_
#define NEWNETWORK_H_

#include "../Component.h"
#include "NetworkTypedefs.h"

class AddressedWord;

class NewNetwork : public Component {

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

  sc_in<bool>   clock;

  // Data coming in to the network, and an extra bit to say whether the data
  // is currently valid.
  DataInput    *dataIn;
  ReadyInput   *validDataIn;

  // Data leaving the network, and an extra bit to say whether the data is
  // currently valid.
  DataOutput   *dataOut;
  ReadyOutput  *validDataOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  // This would work nicely when moving down the hierarchy, but what about when
  // moving up? e.g. Credits pass through a CHANNEL network on the way to the
  // local network, but we don't want them to be routed by the channel value yet.
  NewNetwork(const sc_module_name& name,
          const ComponentID& ID,
          int numInputs,        // Number of inputs this network has
          int numOutputs,       // Number of outputs this network has
          HierarchyLevel level, // Position in the network hierarchy
          Dimension size,       // The physical size of this network (width, height)
          int firstOutput=0,    // The first accessible channel/component/tile
          bool externalConnection=false); // Is there a port to send data on if it
                                          // isn't for any local component?);

  virtual ~NewNetwork();

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

  ReadyInput&  externalValidInput() const;
  ReadyOutput& externalValidOutput() const;

  unsigned int numInputPorts() const;
  unsigned int numOutputPorts() const;

protected:

  // Compute which output of this network will be used by the given address.
  PortIndex getDestination(const ChannelID& address) const;

//==============================//
// Local state
//==============================//

protected:

  const unsigned int numInputs, numOutputs;

  // The channel/component/tile accessible through the first output port.
  // For example, this network may only send to memories, so whilst the target
  // component may have an ID of 8, we may want to use output port 0.
  const unsigned int firstOutput;

  const HierarchyLevel level;

  // Tells whether this network has an extra connection to handle data which
  // isn't for any local component. This extra connection will typically
  // connect to the next level of the network hierarchy.
  const bool externalConnection;

  const Dimension size;

};

#endif /* NEWNETWORK_H_ */
