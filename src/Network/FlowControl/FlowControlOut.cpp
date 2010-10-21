/*
 * FlowControlOut.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#include "FlowControlOut.h"
#include "../../Datatype/AddressedWord.h"

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
        creditCount[i] = FLOW_CONTROL_BUFFER_SIZE;

        // This message is allowed to send even though we have no credits
        // because it is not buffered -- it is immediately consumed to store
        // the return address at the destination.
        dataOut[i].write(dataIn[i].read());
        success = true;
        if(DEBUG) cout << "Sending port claim from component " << id
                       << ", port " << i << endl;
      }
      else if(creditCount[i] > 0) { // Only send if we have at least one credit.
        dataOut[i].write(dataIn[i].read());
        creditCount[i]--;
        success = true;
      }
      else {  // We are not able to send the new data.
//        cout << "Not enough credits to send from port " << i << endl;
        success = false;
      }

      waitingToSend[i] = !success;
      flowControl[i].write(success);
    }
  }
}

void FlowControlOut::receivedCredit() {
  for(int i=0; i<width; i++) {
    if(credits[i].event()) {
      creditCount[i]++;
//      cout << this->name() << " received credit at port " << i << endl;
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
    creditCount(width, 0) {

  id = ID;

  dataIn      = new sc_in<AddressedWord>[width];
  credits   = new sc_in<AddressedWord>[width];
  dataOut     = new sc_out<AddressedWord>[width];
  flowControl = new sc_out<bool>[width];

  SC_METHOD(sendData);
  sensitive << clock.pos();
  dont_initialize();

  SC_METHOD(receivedCredit);
  for(int i=0; i<width; i++) sensitive << credits[i];
  dont_initialize();

}

FlowControlOut::~FlowControlOut() {

}
