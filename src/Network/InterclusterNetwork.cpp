/*
 * InterclusterNetwork.cpp
 *
 *  Created on: 15 Jan 2010
 *      Author: db434
 */

#include "InterclusterNetwork.h"

void InterclusterNetwork::routeRequests() {
  route(requestsIn, requestsOut);
}

void InterclusterNetwork::routeResponses() {
  route(responsesIn, responsesOut);
}

void InterclusterNetwork::routeData() {
  route(dataIn, dataOut);
}

void InterclusterNetwork::route(sc_in<AddressedWord> *inputs, sc_out<Word> *outputs) {
  // TODO: use a RoutingScheme instead
  for(int i=0; i<numInputs; i++) {
    AddressedWord aw = inputs[i].read();
//    if(input is new && haven't already written to this output) {
      outputs[aw.getChannelID()].write(aw.getPayload());
//    }
  }
}

InterclusterNetwork::InterclusterNetwork(sc_module_name name)
    : Interconnect(name) {

  numInputs    = NUM_CLUSTER_OUTPUTS * (CLUSTERS_PER_TILE + MEMS_PER_TILE);
  numOutputs   = NUM_CLUSTER_INPUTS  * (CLUSTERS_PER_TILE + MEMS_PER_TILE);

  responsesOut = new sc_out<Word>[numInputs];
  requestsIn   = new sc_in<AddressedWord>[numInputs];
  responsesIn  = new sc_in<AddressedWord>[numOutputs];
  dataIn       = new sc_in<AddressedWord>[numInputs];
  requestsOut  = new sc_out<Word>[numOutputs];
  dataOut      = new sc_out<Word>[numOutputs];

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
