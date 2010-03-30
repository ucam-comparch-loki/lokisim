/*
 * InterclusterNetwork.cpp
 *
 *  Created on: 15 Jan 2010
 *      Author: db434
 */

#include "InterclusterNetwork.h"
#include "Routing Schemes/RoutingSchemeFactory.h"

const int InterclusterNetwork::numInputs  =
    NUM_CLUSTER_OUTPUTS * COMPONENTS_PER_TILE;

const int InterclusterNetwork::numOutputs =
    NUM_CLUSTER_INPUTS  * COMPONENTS_PER_TILE;

void InterclusterNetwork::routeRequests() {
  route(requestsIn, requestsOut, numInputs, sentRequests);
}

void InterclusterNetwork::routeResponses() {
  route(responsesIn, responsesOut, numOutputs, sentResponses);
}

void InterclusterNetwork::routeData() {
  route(dataIn, dataOut, numInputs, sentData);
}

void InterclusterNetwork::route(input_port *inputs, output_port *outputs,
                                int length, std::vector<bool>& sent) {

  router->route(inputs, outputs, length, sent);

}

InterclusterNetwork::InterclusterNetwork(sc_module_name name) :
    Interconnect(name),
    sentRequests(numOutputs),
    sentResponses(numInputs),
    sentData(numOutputs) {

  responsesOut = new output_port[numInputs];
  requestsIn   = new input_port[numInputs];
  responsesIn  = new input_port[numOutputs];
  dataIn       = new input_port[numInputs];
  requestsOut  = new output_port[numOutputs];
  dataOut      = new output_port[numOutputs];

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
