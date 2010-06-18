/*
 * InterclusterNetwork.cpp
 *
 *  Created on: 15 Jan 2010
 *      Author: db434
 */

#include "InterclusterNetwork.h"
#include "Routing Schemes/RoutingSchemeFactory.h"
#include "../Datatype/Data.h"

const int InterclusterNetwork::numInputs  =
    NUM_CLUSTER_OUTPUTS * COMPONENTS_PER_TILE;

const int InterclusterNetwork::numOutputs =
    NUM_CLUSTER_INPUTS  * COMPONENTS_PER_TILE;

void InterclusterNetwork::routeRequests() {
  route(requestsIn, requestsOut, numInputs, sentRequests, true);

  // Some requests may have been blocked, meaning they can be responded to now.
  // Notify the routeResponses method so it can send the responses.
  sendResponses.write(!sendResponses.read());
}

/* This method can be called either because there are responses being sent
 * into the network and they need routing, or because requests have just
 * finished routing, and some of them might have blocked, needing a response. */
void InterclusterNetwork::routeResponses() {

  if(sendResponses.event()) {
    for(unsigned int i=0; i<blockedRequests.size(); i++) {
      // Send NACKs to any ports that sent requests which were blocked
      if(blockedRequests[i]) {
        responsesOut[i].write(Data(0));
      }
    }
  }
  else {
    route(responsesIn, responsesOut, numOutputs, sentResponses, false);
  }

}

void InterclusterNetwork::routeData() {
  // This is the only routing we are interested in data for.
  route(dataIn, dataOut, numInputs, sentData, false, true);
}

void InterclusterNetwork::route(input_port inputs[],
                                output_port outputs[],
                                int length,
                                std::vector<bool>& sent,
                                bool requests,
                                bool instrumentation) {

  // Since routers aren't components, and only define behaviour, we only need
  // one in the whole system.
  static RoutingScheme* router = RoutingSchemeFactory::makeRoutingScheme();

  // Only send the vector if we are dealing with requests.
  std::vector<bool>* pointer = requests ? &blockedRequests : NULL;

  router->route(inputs, outputs, length, sent, pointer, instrumentation);

}

InterclusterNetwork::InterclusterNetwork(sc_module_name name) :
    Interconnect(name),
    sentRequests(numOutputs),
    sentResponses(numInputs),
    sentData(numOutputs),
    blockedRequests(numOutputs) {

  responsesOut = new output_port[numInputs];
  requestsIn   = new input_port[numInputs];
  responsesIn  = new input_port[numOutputs];
  dataIn       = new input_port[numInputs];
  requestsOut  = new output_port[numOutputs];
  dataOut      = new output_port[numOutputs];

  SC_METHOD(routeRequests);
  for(int i=0; i<numInputs; i++)  sensitive << requestsIn[i];
  dont_initialize();

  SC_METHOD(routeResponses);
  for(int i=0; i<numOutputs; i++) sensitive << responsesIn[i];
  sensitive << sendResponses;
  dont_initialize();

  SC_METHOD(routeData);
  for(int i=0; i<numInputs; i++)  sensitive << dataIn[i];
  dont_initialize();

}

InterclusterNetwork::~InterclusterNetwork() {

}
