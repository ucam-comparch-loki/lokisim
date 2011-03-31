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
  return canSendData[numOutputs-1];
}

sc_out<bool>& Network::externalReadyOutput() const {
  return canReceiveData[numInputs-1];
}

Network::Network(sc_module_name name,
                 ComponentID ID,
                 ChannelID lowestID,
                 ChannelID highestID,
                 int numInputs,
                 int numOutputs,
                 int networkType) :
  RoutingComponent(name, ID, numInputs+1, numOutputs+1, NETWORK_BUFFER_SIZE, networkType) {

  startID          = lowestID;
  endID            = highestID;
  idsPerChannel    = (highestID - lowestID + 1)/numOutputs;
  offNetworkOutput = numOutputs;

}

Network::~Network() {

}
