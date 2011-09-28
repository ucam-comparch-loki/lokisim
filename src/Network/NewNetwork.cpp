/*
 * NewNetwork.cpp
 *
 *  Created on: 2 Nov 2010
 *      Author: db434
 */

#include "NewNetwork.h"
#include "../Datatype/AddressedWord.h"

PortIndex NewNetwork::getDestination(const ChannelID& address) const {
  PortIndex port;

  // Access a different part of the address depending on where in the network
  // we are.
  switch(level) {
    case TILE : port = address.getTile(); break;
    case COMPONENT : {
      if(externalConnection && (address.getTile() != id.getTile()))
        return numOutputs-1;
      else
        port = address.getPosition();
      break;
    }
    case CHANNEL : {
      if(externalConnection && ((address.getTile() != id.getTile()) ||
                                (address.getPosition() != id.getPosition())))
        return numOutputs-1;
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

DataInput& NewNetwork::externalInput() const {
  assert(externalConnection);
  return dataIn[numInputs-1];
}

DataOutput& NewNetwork::externalOutput() const {
  assert(externalConnection);
  return dataOut[numOutputs-1];
}

ReadyInput& NewNetwork::externalValidInput() const {
  assert(externalConnection);
  return validDataIn[numInputs-1];
}

ReadyOutput& NewNetwork::externalValidOutput() const {
  assert(externalConnection);
  return validDataOut[numOutputs-1];
}

unsigned int NewNetwork::numInputPorts()  const {return numInputs;}
unsigned int NewNetwork::numOutputPorts() const {return numOutputs;}

NewNetwork::NewNetwork(const sc_module_name& name,
    const ComponentID& ID,
    int numInputs,        // Number of inputs this network has
    int numOutputs,       // Number of outputs this network has
    HierarchyLevel level, // Position in the network hierarchy
    Dimension size,       // The physical size of this network (width, height)
    int firstOutput,      // The first accessible channel/component/tile
    bool externalConnection) : // Is there a port to send data on if it
                               // isn't for any local component?
    Component(name, ID),
    numInputs(externalConnection ? (numInputs+1) : numInputs),
    numOutputs(externalConnection ? (numOutputs+1) : numOutputs),
    firstOutput(firstOutput),
    level(level),
    externalConnection(externalConnection),
    size(size) {

  assert(numInputs > 0);
  assert(numOutputs > 0);

  dataIn       = new DataInput[this->numInputs];
  validDataIn  = new ReadyInput[this->numInputs];

  dataOut      = new DataOutput[this->numOutputs];
  validDataOut = new ReadyOutput[this->numOutputs];

}

NewNetwork::~NewNetwork() {
  delete[] dataIn;   delete[] validDataIn;
  delete[] dataOut;  delete[] validDataOut;
}
