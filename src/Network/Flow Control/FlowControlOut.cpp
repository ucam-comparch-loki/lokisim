/*
 * FlowControlOut.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#include "FlowControlOut.h"

void FlowControlOut::newCycle() {
  Array<bool> response = responses.read();

  for(int i=0; i<response.length(); i++) {
    allowedToSend(i, response.get(i));
  }

  dataOut.write(toSend);
  flowControl.write(canSend);
}

/* The method to be overwritten in subclasses.
 * This implementation represents acknowledgement flow control, but it could be
 * more complex and, for example, keep a counter to implement credit schemes. */
void FlowControlOut::allowedToSend(int position, bool isAllowed) {
  if(isAllowed) toSend.put(position, dataIn.read().get(position));
  canSend.put(position, isAllowed);
}

void FlowControlOut::sendRequests() {
  for(int i=0; i<toSend.length(); i++) {
    if(!(dataIn.read().get(i) == toSend.get(i))) {
      Request r(id*toSend.length() + i);
      AddressedWord req(r, dataIn.read().get(i).getChannelID());
      requests.put(i, req);
    }
  }
  requestsOut.write(requests);
}

FlowControlOut::FlowControlOut(sc_core::sc_module_name name, int ID, int width) :
    Component(name, ID),
    canSend(width),
    toSend(width) {

  bool f = false;   // Needed so it can be passed by reference

  // Initialise canSend so nothing is allowed to send
  for(int i=0; i<width; i++) {
    canSend.put(i, f);
  }

  SC_METHOD(newCycle);
  sensitive << clock.pos();
  dont_initialize();

  SC_METHOD(sendRequests);
  sensitive << dataIn;
  dont_initialize();

}

FlowControlOut::~FlowControlOut() {

}
