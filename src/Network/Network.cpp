/*
 * Network.cpp
 *
 *  Created on: 2 Nov 2010
 *      Author: db434
 */

#include "Network.h"
#include "../Datatype/AddressedWord.h"

PortIndex Network::getDestination(const ChannelID& address) const {
  // Access a different part of the address depending on where in the network
  // we are.
  switch(level) {
    case TILE : return address.getTile();
    case COMPONENT : {
      if(externalConnection && (address.getTile() != id.getTile()))
        return numOutputs-1;
      else
        return address.getPosition();
    }
    case CHANNEL : {
      if(externalConnection && ((address.getTile() != id.getTile()) ||
                                (address.getPosition() != id.getPosition())))
        return numOutputs-1;
      else
        return address.getChannel();
    }
    case NONE : return 0;
  }

  // Add support for multicast in here?
}

DataInput& Network::externalInput() const {
  assert(externalConnection);
  return dataIn[numInputs-1];
}

DataOutput& Network::externalOutput() const {
  assert(externalConnection);
  return dataOut[numOutputs-1];
}

ReadyInput& Network::externalValidInput() const {
  assert(externalConnection);
  return validDataIn[numInputs-1];
}

ReadyOutput& Network::externalValidOutput() const {
  assert(externalConnection);
  return validDataOut[numOutputs-1];
}

ReadyInput& Network::externalReadyInput() const {
  assert(externalConnection);
  return ackDataOut[numOutputs-1];
}

ReadyOutput& Network::externalReadyOutput() const {
  assert(externalConnection);
  return ackDataIn[numInputs-1];
}

int Network::numInputPorts() const {
  return numInputs;
}

int Network::numOutputPorts() const {
  return numOutputs;
}

Network::Network(sc_module_name name,
    const ComponentID& ID,
    int numInputs,        // Number of inputs this network has
    int numOutputs,       // Number of outputs this network has
    HierarchyLevel level, // Position in the network hierarchy
    Dimension size,       // The physical size of this network (width, height)
    bool externalConnection) : // Is there a port to send data on if it
                               // isn't for any local component?
  Component(name, ID) {

  this->numInputs  = externalConnection ? numInputs+1 : numInputs;
  this->numOutputs = externalConnection ? numOutputs+1 : numOutputs;
  this->level = level;
  this->externalConnection = externalConnection;
  this->size = size;

  dataIn       = new DataInput[this->numInputs];
  validDataIn  = new ReadyInput[this->numInputs];
  ackDataIn    = new ReadyOutput[this->numInputs];

  dataOut      = new DataOutput[this->numOutputs];
  validDataOut = new ReadyOutput[this->numOutputs];
  ackDataOut   = new ReadyInput[this->numOutputs];

}

Network::~Network() {
  delete[] dataIn;
  delete[] validDataIn;
  delete[] ackDataIn;

  delete[] dataOut;
  delete[] validDataOut;
  delete[] ackDataOut;
}
