/*
 * FlowControlIn.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#include "FlowControlIn.h"

/* If the component is allowing data in, and we have data to send, send it. */
void FlowControlIn::receivedFlowControl() {
  for(int i=0; i<width; i++) {
    if(flowControl[i].read() && !(buffers[i].isEmpty())) {
      dataOut[i].write(buffers.read(i));
    }
  }
}

/* Respond to all new requests to send data to this component. */
void FlowControlIn::receivedRequests() {
  for(int i=0; i<width; i++) {
    Request r = static_cast<Request>(requests[i].read());

    // Only send a response if the request is new
    if(requests[i].event()) {
      if(DEBUG) cout << "Received request at input " << i << ": ";

      if(acceptRequest(r, i)) {
        Data d(1);    // Accept
        AddressedWord aw(d, r.getReturnID());
        responses[i].write(aw);

        if(DEBUG) cout << "accepted." << endl;
      }
      else {
        Data d(0);    // Deny
        AddressedWord aw(d, r.getReturnID());
        responses[i].write(aw);

        if(DEBUG) cout << "denied." << endl;
      }
    }
  }
}

/* Put any new data into the buffers. Since we approved the request to send
 * data, it is known that there is enough room. */
void FlowControlIn::receivedData() {
  for(int i=0; i<width; i++) {
    // Only write a value if it is new
    if(dataIn[i].event()) {
      buffers.write(dataIn[i].read(), i);
      tryToSend.write(!tryToSend.read());
    }
  }
}

/* Determine whether the request should be granted. This method should be
 * overridden to implement cut-through/wormhole routing, etc. Current
 * method is store-and-forward (must be room in buffer for whole request).
 * TODO: Don't accept multiple requests to the same port. */
bool FlowControlIn::acceptRequest(Request r, int input) {
  return buffers[input].remainingSpace() >= r.getNumFlits();
}

void FlowControlIn::setup() {

  dataIn      = new sc_in<Word>[width];
  requests    = new sc_in<Word>[width];
  flowControl = new sc_in<bool>[width];
  dataOut     = new sc_out<Word>[width];
  responses   = new sc_out<AddressedWord>[width];

  SC_METHOD(receivedFlowControl);
  sensitive << tryToSend;
  for(int i=0; i<width; i++) sensitive << flowControl[i];
  dont_initialize();

  SC_METHOD(receivedRequests);
  for(int i=0; i<width; i++) sensitive << requests[i];
  dont_initialize();

  SC_METHOD(receivedData);
  for(int i=0; i<width; i++) sensitive << dataIn[i];
  dont_initialize();

}

FlowControlIn::FlowControlIn(sc_module_name name, int width) :
    Component(name),
    buffers(width, FLOW_CONTROL_BUFFER_SIZE),
    width(width) {

  setup();

}

FlowControlIn::~FlowControlIn() {

}
