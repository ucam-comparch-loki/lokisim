/*
 * FlowControlIn.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#include "FlowControlIn.h"

void FlowControlIn::receivedFlowControl() {
  Array<bool> control = flowControl.read();

  for(int i=0; i<control.length(); i++) {
    if(control.get(i) && !buffers.at(i).isEmpty()) {
      toSend.put(i, buffers.at(i).read());
    }
  }

  dataOut.write(toSend);
}

void FlowControlIn::receivedRequests() {
  for(unsigned int i=0; i<buffers.size(); i++) {
    Request r = static_cast<Request>(requests.read().get(i).getPayload());
    if(/*request is new &&*/ !buffers.at(i).remainingSpace() >= r.getNumFlits()) {
      Data d(1);    // Accept
      AddressedWord aw(d, r.getReturnID());
      response.put(i, aw);
    }
    else {
      Data d(0);    // Deny
      AddressedWord aw(d, r.getReturnID());
      response.put(i, aw);
    }
  }
}

void FlowControlIn::receivedData() {
  Array<AddressedWord> data = dataIn.read();
  for(int i=0; i<data.length(); i++) {
    /* if(data.get(i) is new) */ buffers.at(i).write(data.get(i));
  }
}

FlowControlIn::FlowControlIn(sc_core::sc_module_name name, int ID, int width) :
    Component(name, ID),
    buffers(width),
    toSend(width),
    response(width) {

  for(int i=0; i<width; i++) {
    Buffer<AddressedWord>* b = new Buffer<AddressedWord>(FLOW_CONTROL_BUFFER_SIZE);
    buffers.push_back(*b);
  }

  SC_METHOD(receivedFlowControl);
  sensitive << flowControl;
  dont_initialize();

  SC_METHOD(receivedRequests);
  sensitive << responses;
  dont_initialize();

  SC_METHOD(receivedData);
  sensitive << dataIn;
  dont_initialize();

}

FlowControlIn::~FlowControlIn() {

}
