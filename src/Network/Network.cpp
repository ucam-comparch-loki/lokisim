/*
 * Network.cpp
 *
 *  Created on: 2 Nov 2010
 *      Author: db434
 */

#include "Network.h"
#include "../Exceptions/InvalidOptionException.h"

PortIndex Network::getDestination(const ChannelID& address) const {
  PortIndex port;

  // Access a different part of the address depending on where in the network
  // we are.
  switch(level) {
    case TILE :
      port = address.getTile() - firstOutput;
      break;
    case COMPONENT : {
      if(externalConnection && (address.getTile() != id.getTile()))
        port = numOutputPorts()-1;
      else
        port = address.getPosition() - firstOutput;
      break;
    }
    case CHANNEL : {
      if(externalConnection && ((address.getTile() != id.getTile()) ||
                                (address.getPosition() != id.getPosition())))
        port = numOutputPorts()-1;
      else
        port = address.getChannel() - firstOutput;
      break;
    }
    case NONE :
      port = 0;
      break;
    default :
      port = 0;
      throw InvalidOptionException("network scope", level);
      break;
  }

  // Make an adjustment in case this network is not capable of sending to
  // address 0.
  return port;
}

DataInput& Network::externalInput() const {
  assert(externalConnection);
  return iData[numInputPorts()-1];
}

DataOutput& Network::externalOutput() const {
  assert(externalConnection);
  return oData[numOutputPorts()-1];
}

unsigned int Network::numInputPorts()  const {return iData.length();}
unsigned int Network::numOutputPorts() const {return oData.length();}

Network::Network(const sc_module_name& name,
    const ComponentID& ID,
    int numInputs,        // Number of inputs this network has
    int numOutputs,       // Number of outputs this network has
    HierarchyLevel level, // Position in the network hierarchy
    int firstOutput,      // The first accessible channel/component/tile
    bool externalConnection) : // Is there a port to send data on if it
                               // isn't for any local component?
    Component(name, ID),
    firstOutput(firstOutput),
    level((numOutputs > 1) ? level : NONE),
    externalConnection(externalConnection) {

  assert(numInputs > 0);
  assert(numOutputs > 0);

  unsigned int totalInputs  = externalConnection ? (numInputs+1) : numInputs;
  unsigned int totalOutputs = externalConnection ? (numOutputs+1) : numOutputs;
  iData.init(totalInputs);
  oData.init(totalOutputs);

}
