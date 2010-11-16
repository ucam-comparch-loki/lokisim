/*
 * FlowControlOut.cpp
 *
 *  Created on: 8 Nov 2010
 *      Author: db434
 */

#include "FlowControlOut.h"
#include "../NetworkHierarchy.h"

void FlowControlOut::mainLoop() {
  while(true) {
    wait(dataIn.default_event());
    sendData();
  }
}

void FlowControlOut::sendData() {

  // Don't allow any more data until this send is complete.
  flowControlOut.write(false);

  // Wait for the ready signal to be set, if it is not already.
  if(!readyIn.read()) wait(readyIn.posedge_event());

  bool success; // Record whether the data was sent successfully.

  if(dataIn.read().portClaim()) {
    // Reset our credit count if we are communicating with someone new.
    // We currently assume that the end buffer will be empty when we set
    // up the connection, but this may not be the case later on.
    // Warning: flow control currently doesn't have buffering, so this may
    // not always be valid.
    creditCount = CHANNEL_END_BUFFER_SIZE;

    // This message is allowed to send even if we have no credits
    // because it is not buffered -- it is immediately consumed to store
    // the return address at the destination.
    dataOut.write(dataIn.read());
    success = true;
//      if(DEBUG) cout << "Sending port claim from component " << id
//                     << ", port " << i << endl;
  }
  else {
    if(creditCount <= 0) {  // We are not able to send the new data yet.
      if(DEBUG) cout << "Can't send from "
          << NetworkHierarchy::portLocation(id, false) << ": no credits." <<  endl;

      // Wait until we receive a credit.
      wait(creditsIn.default_event());
    }

    dataOut.write(dataIn.read());
    creditCount--;

    Instrumentation::networkTraffic(id, dataIn.read().channelID());

    if(DEBUG) cout << "Network sending " << dataIn.read().payload() << " from "
        << NetworkHierarchy::portLocation(id, false) << " to "
        << NetworkHierarchy::portLocation(dataIn.read().channelID(), true)
        << endl;
  }

  flowControlOut.write(true);

}

void FlowControlOut::receivedCredit() {
  creditCount++;

  if(DEBUG) cout << "Received credit at port "
       << NetworkHierarchy::portLocation(id, false) << endl;
}

FlowControlOut::FlowControlOut(sc_module_name name, ComponentID ID) :
    Component(name, ID) {

  id = ID;

  creditCount = 0;

  SC_THREAD(mainLoop);

  SC_METHOD(receivedCredit);
  sensitive << creditsIn;
  dont_initialize();

  // Start execution allowing all inputs.
  flowControlOut.initialize(true);
  readyOut.initialize(true);

  end_module(); // Needed because we're using a different Component constructor

}

FlowControlOut::~FlowControlOut() {

}
