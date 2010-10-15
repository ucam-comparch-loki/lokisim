/*
 * FlowControlOut.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#include "FlowControlOut.h"
#include "../../Datatype/AddressedWord.h"
#include "../../Datatype/Data.h"
#include "../../Datatype/Request.h"

void FlowControlOut::receivedResponses() {
  for(int i=0; i<width; i++) {
    if(responses[i].event()) {
      Data d = static_cast<Data>(responses[i].read().payload());
      allowedToSend(i, d.data());
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

  waitingToSend[position] = !isAllowed;  // If denied, send another request
  flowControl[position].write(isAllowed);
}

void FlowControlOut::sendData() {
  for(int i=0; i<width; i++) {
    if(dataIn[i].event() || waitingToSend[i]) {
      bool success; // Record whether the data was sent successfully.

      if(dataIn[i].read().portClaim()) {
        // Reset our credit count if we are communicating with someone new.
        // We currently assume that the end buffer will be empty when we set
        // up the connection, but this may not be the case later on.
        // Warning: flow control currently doesn't have buffering, so this may
        // not always be valid.
        // TODO: output channels don't correspond to connections. Need a credit
        // counter for each entry in the channel mapping table.
        credits[i] = FLOW_CONTROL_BUFFER_SIZE;

        // This message is allowed to send even though we have no credits
        // because it is not buffered -- it is immediately consumed to store
        // the return address at the destination.
        dataOut[i].write(dataIn[i].read());
        success = true;
        cout << "Sending port claim from component " << id << ", port " << i << " " << credits[i] << endl;
      }
      else if(credits[i] > 0) { // Only send if we have at least one credit.
        dataOut[i].write(dataIn[i].read());
        credits[i]--;
        success = true;
        cout << "Sending normal data from component " << id << ", port " << i << " " << credits[i] << endl;
      }
      else {  // We are not able to send the new data.
        success = false;
        cout << "Unable to send data from component " << id << ", port " << i << endl;
        cout << dataIn[i].read() << " " << i << " " << credits[i] << endl;
      }

      waitingToSend[i] = !success;
      flowControl[i].write(success);
    }
  }
}

void FlowControlOut::receivedFlowControl() {
  for(int i=0; i<width; i++) {
    if(responses[i].event()) {
      credits[i]++;
      cout << "Component " << id << ", port " << i << " received credit." << endl;
    }
  }
}

void FlowControlOut::sendRequests() {
  for(int i=0; i<width; i++) {
    if(dataIn[i].event() || (clock.event() && waitingToSend[i])) {
      Request r(id*width + i);
      AddressedWord req(r, dataIn[i].read().channelID());
      requests[i].write(req);
      waitingToSend[i] = false;
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

FlowControlOut::FlowControlOut(sc_module_name name, int ID, int width) :
    Component(name),
    width(width),
    waitingToSend(width, false),
    credits(width, 0) {

  id = ID;

  dataIn      = new sc_in<AddressedWord>[width];
  responses   = new sc_in<AddressedWord>[width];
  dataOut     = new sc_out<AddressedWord>[width];
  requests    = new sc_out<AddressedWord>[width];
  flowControl = new sc_out<bool>[width];

//  SC_METHOD(receivedResponses);
//  for(int i=0; i<width; i++) sensitive << responses[i];
//  dont_initialize();

//  SC_METHOD(sendRequests);
//  for(int i=0; i<width; i++) sensitive << dataIn[i];
//  sensitive << clock.pos();
//  dont_initialize();

  SC_METHOD(sendData);
  sensitive << clock.pos();
  dont_initialize();

  SC_METHOD(receivedFlowControl);
  for(int i=0; i<width; i++) sensitive << responses[i];
  dont_initialize();

}

FlowControlOut::~FlowControlOut() {

}
