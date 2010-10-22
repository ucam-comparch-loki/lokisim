/*
 * InterclusterNetwork.cpp
 *
 *  Created on: 15 Jan 2010
 *      Author: db434
 */

#include "InterclusterNetwork.h"
#include "RoutingSchemes/RoutingSchemeFactory.h"

const int InterclusterNetwork::numInputs  =
    NUM_CLUSTER_OUTPUTS * COMPONENTS_PER_TILE;

const int InterclusterNetwork::numOutputs =
    NUM_CLUSTER_INPUTS  * COMPONENTS_PER_TILE;

/* This method can be called either because there are responses being sent
 * into the network and they need routing, or because requests have just
 * finished routing, and some of them might have blocked, needing a response. */
void InterclusterNetwork::routeCredits() {
  route(creditsIn, creditsOut, numOutputs, sentCredits);
}

void InterclusterNetwork::routeData() {
  // This is the only routing we are interested in data for.
  route(dataIn, dataOut, numInputs, sentData, true);
}

void InterclusterNetwork::route(input_port inputs[],
                                output_port outputs[],
                                int length,
                                std::vector<bool>& sent,
                                bool instrumentation) {

  // Since routers aren't Components, and only define behaviour, we only need
  // one in the whole system.
  static RoutingScheme* router = RoutingSchemeFactory::makeRoutingScheme();

  router->route(inputs, outputs, length, sent, instrumentation);

}

InterclusterNetwork::InterclusterNetwork(sc_module_name name) :
    Interconnect(name),
    sentCredits(numInputs),
    sentData(numOutputs) {

  dataIn     = new input_port[numInputs];
  dataOut    = new output_port[numOutputs];
  creditsIn  = new input_port[numOutputs];
  creditsOut = new output_port[numInputs];

  SC_METHOD(routeCredits);
  for(int i=0; i<numOutputs; i++) sensitive << creditsIn[i];
  dont_initialize();

  SC_METHOD(routeData);
  for(int i=0; i<numInputs; i++)  sensitive << dataIn[i];
  dont_initialize();

}

InterclusterNetwork::~InterclusterNetwork() {
  delete[] dataIn;
  delete[] dataOut;
  delete[] creditsIn;
  delete[] creditsOut;
}
