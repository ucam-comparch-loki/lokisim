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

ReadyInput& Network::externalReadyInput() const {
  assert(externalConnection);
  return canSendData[numOutputs-1];
}

ReadyOutput& Network::externalReadyOutput() const {
  assert(externalConnection);
  return canReceiveData[numInputs-1];
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

  dataIn         = new DataInput[this->numInputs];
  dataOut        = new DataOutput[this->numOutputs];
  canSendData    = new ReadyInput[this->numOutputs];
  canReceiveData = new ReadyOutput[this->numInputs];

  // We start off accepting any input.
  for(int i=0; i<this->numInputs; i++) canReceiveData[i].initialize(true);

  // Need to call end_module in every subclass.
//  end_module();

}

Network::~Network() {
  delete[] dataIn;
  delete[] dataOut;
  delete[] canReceiveData;
  delete[] canSendData;
}
