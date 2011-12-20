/*
 * UnclockedNetwork.cpp
 *
 *  Created on: 27 Apr 2011
 *      Author: db434
 */

#include "UnclockedNetwork.h"
#include "Network.h"

void UnclockedNetwork::dataArrived() {
  newData.notify();
}

void UnclockedNetwork::generateClock() {
  // The clock goes high whenever data arrives, and low immediately afterwards.
  if(clockSig.read()) next_trigger(newData);
  else                next_trigger(clockSig.default_event());

  clockSig.write(!clockSig.read());
}

UnclockedNetwork::UnclockedNetwork(sc_module_name name, Network* network) :
    Component(name),
    network_(network) {

  assert(network != NULL);

  int inputs   = network->numInputPorts();
  int outputs  = network->numOutputPorts();

  dataIn       = new DataInput[inputs];
  dataOut      = new DataOutput[outputs];

  for(int i=0; i<inputs; i++)
    network->dataIn[i](dataIn[i]);

  for(int i=0; i<outputs; i++)
    network->dataOut[i](dataOut[i]);

  network->clock(clockSig);

  SC_METHOD(generateClock);
  sensitive << newData;
  dont_initialize();

  SC_METHOD(dataArrived);
  for(int i=0; i<inputs; i++) sensitive << dataIn[i];
  dont_initialize();

}

UnclockedNetwork::~UnclockedNetwork() {
  delete   network_;
  delete[] dataIn;
  delete[] dataOut;
}
