/*
 * UnclockedNetwork.cpp
 *
 *  Created on: 27 Apr 2011
 *      Author: db434
 */

#include "UnclockedNetwork.h"
#include "Network.h"
#include "../Utility/Assert.h"

void UnclockedNetwork::dataArrived() {
  newData.notify();
}

void UnclockedNetwork::generateClock() {
  // The clock goes high whenever data arrives, and low immediately afterwards.
  if (clockSig.read()) next_trigger(newData);
  else                 next_trigger(clockSig.default_event());

  clockSig.write(!clockSig.read());
}

UnclockedNetwork::UnclockedNetwork(sc_module_name name, Network* network) :
    Component(name),
    network_(network) {

  loki_assert(network != NULL);

  int inputs   = network->numInputPorts();
  int outputs  = network->numOutputPorts();

  iData.init(inputs);
  oData.init(outputs);

  for(int i=0; i<inputs; i++)
    network->iData[i](iData[i]);

  for(int i=0; i<outputs; i++)
    network->oData[i](oData[i]);

  network->clock(clockSig);

  SC_METHOD(generateClock);
  sensitive << newData;
  dont_initialize();

  SC_METHOD(dataArrived);
  for(int i=0; i<inputs; i++) sensitive << iData[i];
  dont_initialize();

}

UnclockedNetwork::~UnclockedNetwork() {
  delete   network_;
}
