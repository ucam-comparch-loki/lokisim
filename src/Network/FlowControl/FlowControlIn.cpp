/*
 * FlowControlIn.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#include "FlowControlIn.h"

/* Respond to all new requests to send data to this component. */
void FlowControlIn::receivedRequests() {
  for(int i=0; i<width; i++) {
    Request r = static_cast<Request>(requests[i].read());

    // Only send a response if the request is new
    if(requests[i].event()) {

      if(acceptRequest(r, i)) {
        Data d(1);    // Accept
        AddressedWord aw(d, r.getReturnID());
        responses[i].write(aw);

        // Store the number of flits we're expecting. This allows us to block
        // any new requests until the transaction is complete.
        flitsRemaining[i] = r.getNumFlits();
      }
      else {
        Data d(0);    // Deny
        AddressedWord aw(d, r.getReturnID());
        responses[i].write(aw);
      }
    }
  }
}

/* Put any new data into the buffers. Since we approved the request to send
 * data, it is known that there is enough room. */
void FlowControlIn::receivedData() {
  for(int i=0; i<width; i++) {
    // Only write a value if it is new and it is expected
    if(dataIn[i].event() && (flitsRemaining[i] > 0)) {

      // TODO: select output channel based on information received.
      // Allows virtual channels/multiplexing/etc
      dataOut[i].write(dataIn[i].read());
      flitsRemaining[i] -= 1;

    }
  }
}

/* Determine whether the request should be granted. This method should be
 * overridden to implement cut-through/wormhole routing, etc. Current
 * method is store-and-forward (must be room in buffer for whole request). */
bool FlowControlIn::acceptRequest(Request r, int input) {
  // Accept a request if there is space, and the port is free.
  bool result = (flowControl[input] >= r.getNumFlits()) &&
                (flitsRemaining[input] == 0);

  return result;
}

void FlowControlIn::setup() {

  dataIn      = new sc_in<Word>[width];
  requests    = new sc_in<Word>[width];
  flowControl = new sc_in<int>[width];
  dataOut     = new sc_out<Word>[width];
  responses   = new sc_out<AddressedWord>[width];

  SC_METHOD(receivedRequests);
  for(int i=0; i<width; i++) sensitive << requests[i];
  dont_initialize();

  SC_METHOD(receivedData);
  for(int i=0; i<width; i++) sensitive << dataIn[i];
  dont_initialize();

}

FlowControlIn::FlowControlIn(sc_module_name name, int width) :
    Component(name),
    width(width),
    flitsRemaining(width, 0) {

  setup();

}

FlowControlIn::~FlowControlIn() {

}