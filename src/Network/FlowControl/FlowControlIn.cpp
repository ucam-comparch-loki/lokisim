/*
 * FlowControlIn.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#include "FlowControlIn.h"
#include "../../Datatype/AddressedWord.h"
#include "../../Datatype/Data.h"
#include "../../Datatype/Request.h"

/* Respond to all new requests to send data to this component. */
void FlowControlIn::receivedRequests() {
  for(int i=0; i<width; i++) {
    Request r = static_cast<Request>(requests[i].read().payload());

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
    // Only take action if an input value is new.
    if(dataIn[i].event()) {// && (flitsRemaining[i] > 0)) {
      if(dataIn[i].read().portClaim()) {
        // Set the return address so we can send flow control.
        returnAddresses[i] = dataIn[i].read().payload().toInt();

        if(DEBUG) cout << this->name() << ": port " << i << " was claimed by "
                       << returnAddresses[i] << endl;

        // Send initial flow control data here if necessary.
      }
      else {
        // Pass the value to the component.
        dataOut[i].write(dataIn[i].read().payload());
//        flitsRemaining[i] -= 1;
      }
    }
  }
}

void FlowControlIn::receivedFlowControl() {
  for(int i=0; i<width; i++) {
    // Send a credit if the amount of space has increased, and someone is
    // communicating with this port.
    if((flowControl[i].event()/*.read() > bufferSpace[i]*/) && (returnAddresses[i] != -1)) {
      AddressedWord aw(Data(1), returnAddresses[i]);
      responses[i].write(aw);
      cout << "Sent a credit from " << this->name() << ", port " << i << endl;
    }

    bufferSpace[i] = flowControl[i].read();
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

FlowControlIn::FlowControlIn(sc_module_name name, int width) :
    Component(name),
    width(width),
    flitsRemaining(width, 0),
    returnAddresses(width, -1),
    bufferSpace(width, 0) {

  dataIn      = new sc_in<AddressedWord>[width];
  requests    = new sc_in<AddressedWord>[width];
  flowControl = new sc_in<int>[width];
  dataOut     = new sc_out<Word>[width];
  responses   = new sc_out<AddressedWord>[width];

//  SC_METHOD(receivedRequests);
//  for(int i=0; i<width; i++) sensitive << requests[i];
//  dont_initialize();

  SC_METHOD(receivedData);
  for(int i=0; i<width; i++) sensitive << dataIn[i];
  dont_initialize();

  SC_METHOD(receivedFlowControl);
  for(int i=0; i<width; i++) sensitive << flowControl[i];
  dont_initialize();

}

FlowControlIn::~FlowControlIn() {

}
