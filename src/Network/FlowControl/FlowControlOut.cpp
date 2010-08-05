/*
 * FlowControlOut.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#include "FlowControlOut.h"
#include "../../Datatype/Data.h"

void FlowControlOut::receivedResponses() {
  for(int i=0; i<width; i++) {
    if(responses[i].event()) {
      Data d = static_cast<Data>(responses[i].read());
      allowedToSend(i, d.getData());
    }
  }
}

/* The method to be overwritten in subclasses.
 * This implementation represents acknowledgement flow control, but it could be
 * more complex and, for example, keep a counter to implement credit schemes. */
void FlowControlOut::allowedToSend(int position, bool isAllowed) {
  if(isAllowed) {
    dataOut[position].write(dataIn[position].read());
  }
  else {
    if(DEBUG) cout<<this->name()<<" received NACK at output "<<position<<endl;
  }

  waitingToRequest[position] = !isAllowed;  // If denied, send another request
  flowControl[position].write(isAllowed);
}

void FlowControlOut::sendRequests() {
  for(int i=0; i<width; i++) {
    if(dataIn[i].event() || (clock.event() && waitingToRequest[i])) {
      Request r(id*width + i);
      AddressedWord req(r, dataIn[i].read().getChannelID());
      requests[i].write(req);
      waitingToRequest[i] = false;
    }
  }
}

/* Initialise so everything is allowed to send. This can't be done in the
 * constructor because the ports aren't connected to anything yet. */
void FlowControlOut::initialise() {
  for(int i=0; i<width; i++) {
    flowControl[i].write(true);
  }
}

void FlowControlOut::setup() {

  dataIn      = new sc_in<AddressedWord>[width];
  responses   = new sc_in<Word>[width];
  dataOut     = new sc_out<AddressedWord>[width];
  requests    = new sc_out<AddressedWord>[width];
  flowControl = new sc_out<bool>[width];

  SC_METHOD(receivedResponses);
  for(int i=0; i<width; i++) sensitive << responses[i];
  dont_initialize();

  SC_METHOD(sendRequests);
  for(int i=0; i<width; i++) sensitive << dataIn[i] << clock.pos();
  dont_initialize();

}

FlowControlOut::FlowControlOut(sc_module_name name, int ID, int width) :
    Component(name),
    width(width),
    waitingToRequest(width, false) {

  id = ID;
  setup();

}

FlowControlOut::~FlowControlOut() {

}
