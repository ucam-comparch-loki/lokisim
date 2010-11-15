/*
 * Network.cpp
 *
 *  Created on: 2 Nov 2010
 *      Author: db434
 */

#include "Network.h"

sc_in<AddressedWord>& Network::externalInput() const {
  return dataIn[numInputs-1];
}

sc_out<AddressedWord>& Network::externalOutput() const {
  return dataOut[numOutputs-1];
}

sc_in<bool>& Network::externalReadyInput() const {
  return readyIn[numOutputs-1];
}

sc_out<bool>& Network::externalReadyOutput() const {
  return readyOut[numInputs-1];
}

Network::Network(sc_module_name name,
                 ComponentID ID,
                 ChannelID lowestID,
                 ChannelID highestID,
                 int numInputs,
                 int numOutputs) :
  RoutingComponent(name, ID, numInputs+1, numOutputs+1, NETWORK_BUFFER_SIZE) {

  startID          = lowestID;
  endID            = highestID;
  idsPerChannel    = (highestID - lowestID + 1)/numOutputs;
  offNetworkOutput = numOutputs;

}

Network::~Network() {

}
