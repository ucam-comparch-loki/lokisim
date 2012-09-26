/*
 * Network.cpp
 *
 *  Created on: 2 Nov 2010
 *      Author: db434
 */

#include "Network.h"
#include "../Datatype/AddressedWord.h"

PortIndex Network::getDestination(const ChannelID& address) const {
  PortIndex port;

  // Access a different part of the address depending on where in the network
  // we are.
  switch(level) {
    case TILE : port = address.getTile(); break;
    case COMPONENT : {
      if(externalConnection && (address.getTile() != id.getTile()))
        return numOutputPorts()-1;
      else
        port = address.getPosition();
      break;
    }
    case CHANNEL : {
      if(externalConnection && ((address.getTile() != id.getTile()) ||
                                (address.getPosition() != id.getPosition())))
        return numOutputPorts()-1;
      else
        port = address.getChannel();
      break;
    }
    case NONE : return 0;
  }

  // Make an adjustment in case this network is not capable of sending to
  // address 0.
  return port - firstOutput;
}

DataInput& Network::externalInput() const {
  assert(externalConnection);
  return dataIn[numInputPorts()-1];
}

DataOutput& Network::externalOutput() const {
  assert(externalConnection);
  return dataOut[numOutputPorts()-1];
}

unsigned int Network::numInputPorts()  const {return dataIn.length();}
unsigned int Network::numOutputPorts() const {return dataOut.length();}

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
    level(level),
    externalConnection(externalConnection) {

  unsigned int totalInputs  = externalConnection ? (numInputs+1) : numInputs;
  unsigned int totalOutputs = externalConnection ? (numOutputs+1) : numOutputs;
  dataIn.init(totalInputs);
  dataOut.init(totalOutputs);

}
