/*
 * FlowControlOut.cpp
 *
 *  Created on: 8 Nov 2010
 *      Author: db434
 */

#include "FlowControlOut.h"
#include "../NetworkHierarchy.h"
#include "../../TileComponents/TileComponent.h"

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
    // This message is allowed to send even if we have no credits
    // because it is not buffered -- it is immediately consumed to store
    // the return address at the destination.
    dataOut.write(dataIn.read());
    success = true;
//      if(DEBUG) cout << "Sending port claim from component " << id
//                     << ", port " << i << endl;

    creditCount--;
    assert(creditCount >= 0 && creditCount <= CHANNEL_END_BUFFER_SIZE);
  }
  else {
    if(creditCount <= 0) {  // We are not able to send the new data yet.
      if(DEBUG) cout << "Can't send from "
          << TileComponent::outputPortString(id) << ": no credits." <<  endl;

      // Wait until we receive a credit.
      wait(creditsIn.default_event());
    }

    if(DEBUG) cout << "Network sending " << dataIn.read().payload() << " from "
        << TileComponent::outputPortString(id) << " to "
        << TileComponent::inputPortString(dataIn.read().channelID())
        << endl;

    dataOut.write(dataIn.read());
    creditCount--;
    assert(creditCount >= 0 && creditCount <= CHANNEL_END_BUFFER_SIZE);

    Instrumentation::networkTraffic(id, dataIn.read().channelID());
  }

  flowControlOut.write(true);

}

void FlowControlOut::receivedCredit() {
  creditCount++;

  if(DEBUG) cout << "Received credit at port "
       << TileComponent::outputPortString(id) << endl;
  assert(creditCount >= 0 && creditCount <= CHANNEL_END_BUFFER_SIZE);
}

FlowControlOut::FlowControlOut(sc_module_name name, ComponentID ID) :
    Component(name, ID) {

  id = ID;

  creditCount = CHANNEL_END_BUFFER_SIZE;

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
