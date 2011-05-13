/*
 * Network.cpp
 *
 *  Created on: 2 Nov 2010
 *      Author: db434
 */

#include "Network.h"
#include "../Datatype/AddressedWord.h"

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
    ComponentID ID,
    int numInputs,        // Number of inputs this network has
    int numOutputs,       // Number of outputs this network has
    Dimension size,       // The physical size of this network (width, height)
    bool externalConnection) : // Is there a port to send data on if it
                               // isn't for any local component?
  Component(name, ID) {

  this->numInputs  = externalConnection ? numInputs+1 : numInputs;
  this->numOutputs = externalConnection ? numOutputs+1 : numOutputs;
  this->externalConnection = externalConnection;
  this->size = size;

  dataIn       = new DataInput[this->numInputs];
  validDataIn  = new ReadyInput[this->numInputs];
  ackDataIn    = new ReadyOutput[this->numInputs];

  dataOut      = new DataOutput[this->numOutputs];
  validDataOut = new ReadyOutput[this->numOutputs];
  ackDataOut   = new ReadyInput[this->numOutputs];

  // Need to call end_module in every subclass.
//  end_module();

}

Network::~Network() {
  delete[] dataIn;
  delete[] validDataIn;
  delete[] ackDataIn;

  delete[] dataOut;
  delete[] validDataOut;
  delete[] ackDataOut;
}
