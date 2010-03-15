/*
 * InterclusterNetwork.cpp
 *
 *  Created on: 15 Jan 2010
 *      Author: db434
 */

#include "InterclusterNetwork.h"
#include "Routing Schemes/RoutingSchemeFactory.h"

void InterclusterNetwork::routeRequests() {
  route(requestsIn, requestsOut, numInputs);
}

void InterclusterNetwork::routeResponses() {
  route(responsesIn, responsesOut, numOutputs);
}

void InterclusterNetwork::routeData() {
  route(dataIn, dataOut, numInputs);
}

void InterclusterNetwork::route(sc_in<AddressedWord> *inputs,
                                sc_out<Word> *outputs, int length) {

  router->route(inputs, outputs, length);

}

InterclusterNetwork::InterclusterNetwork(sc_module_name name)
    : Interconnect(name) {

  numInputs    = NUM_CLUSTER_OUTPUTS * COMPONENTS_PER_TILE;
  numOutputs   = NUM_CLUSTER_INPUTS  * COMPONENTS_PER_TILE;

  responsesOut = new sc_out<Word>[numInputs];
  requestsIn   = new sc_in<AddressedWord>[numInputs];
  responsesIn  = new sc_in<AddressedWord>[numOutputs];
  dataIn       = new sc_in<AddressedWord>[numInputs];
  requestsOut  = new sc_out<Word>[numOutputs];
  dataOut      = new sc_out<Word>[numOutputs];

  router       = RoutingSchemeFactory::makeRoutingScheme();

  SC_METHOD(routeRequests);
  for(int i=0; i<numInputs; i++) sensitive << requestsIn[i];
  dont_initialize();

  SC_METHOD(routeResponses);
  for(int i=0; i<numOutputs; i++) sensitive << responsesIn[i];
  dont_initialize();

  SC_METHOD(routeData);
  for(int i=0; i<numInputs; i++) sensitive << dataIn[i];
  dont_initialize();

}

InterclusterNetwork::~InterclusterNetwork() {

}
