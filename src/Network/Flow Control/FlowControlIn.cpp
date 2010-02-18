/*
 * FlowControlIn.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#include "FlowControlIn.h"

void FlowControlIn::receivedFlowControl() {
  for(int i=0; i<width; i++) {
    if(flowControl[i].read() && !buffers.at(i).isEmpty()) {
      dataOut[i].write(buffers.at(i).read());
    }
  }
}

void FlowControlIn::receivedRequests() {
  for(int i=0; i<width; i++) {
    Request r = static_cast<Request>(requests[i].read());
    if(/*request is new &&*/ !buffers.at(i).remainingSpace() >= r.getNumFlits()) {
      Data d(1);    // Accept
      AddressedWord aw(d, r.getReturnID());
      responses[i].write(aw);
    }
    else {
      Data d(0);    // Deny
      AddressedWord aw(d, r.getReturnID());
      responses[i].write(aw);
    }
  }
}

void FlowControlIn::receivedData() {
  for(int i=0; i<width; i++) {
    /* if(dataIn[i].read() is new) */ buffers.at(i).write(dataIn[i].read());
  }
}

FlowControlIn::FlowControlIn(sc_core::sc_module_name name, int ID, int width) :
    Component(name, ID),
    buffers(width) {

  this->width = width;

  dataIn = new sc_in<Word>[width];
  requests = new sc_in<Word>[width];
  flowControl = new sc_in<bool>[width];
  dataOut = new sc_out<Word>[width];
  responses = new sc_out<AddressedWord>[width];

  for(int i=0; i<width; i++) {
    Buffer<Word>* b = new Buffer<Word>(FLOW_CONTROL_BUFFER_SIZE);
    buffers.push_back(*b);
  }

  SC_METHOD(receivedFlowControl);
  for(int i=0; i<width; i++) sensitive << flowControl[i];
  dont_initialize();

  SC_METHOD(receivedRequests);
  for(int i=0; i<width; i++) sensitive << requests[i];
  dont_initialize();

  SC_METHOD(receivedData);
  for(int i=0; i<width; i++) sensitive << dataIn[i];
  dont_initialize();

}

FlowControlIn::~FlowControlIn() {

}
