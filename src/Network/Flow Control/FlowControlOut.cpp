/*
 * FlowControlOut.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#include "FlowControlOut.h"

void FlowControlOut::newCycle() {
  for(int i=0; i<width; i++) {
    allowedToSend(i, responses[i].read());
  }
}

/* The method to be overwritten in subclasses.
 * This implementation represents acknowledgement flow control, but it could be
 * more complex and, for example, keep a counter to implement credit schemes. */
void FlowControlOut::allowedToSend(int position, bool isAllowed) {
  if(isAllowed) dataOut[position].write(dataIn[position].read());
  flowControl[position].write(isAllowed==1);
}

void FlowControlOut::sendRequests() {
  for(int i=0; i<width; i++) {
    if(!(dataIn[i].read() == dataOut[i].read())) {
      Request r(id*width + i);
      AddressedWord req(r, dataIn[i].read().getChannelID());
      requests[i].write(req);
    }
  }
}

FlowControlOut::FlowControlOut(sc_core::sc_module_name name, int ID, int width) :
    Component(name, ID) {

  this->width = width;

  dataIn = new sc_in<AddressedWord>[width];
  responses = new sc_in<bool>[width];
  dataOut = new sc_out<AddressedWord>[width];
  requests = new sc_out<AddressedWord>[width];
  flowControl = new sc_out<bool>[width];

  // Initialise canSend so nothing is allowed to send
  for(int i=0; i<width; i++) {
    flowControl[i].write(false);
  }

  SC_METHOD(newCycle);
  sensitive << clock.pos();
  dont_initialize();

  SC_METHOD(sendRequests);
  for(int i=0; i<width; i++) sensitive << dataIn[i];
  dont_initialize();

}

FlowControlOut::~FlowControlOut() {

}
