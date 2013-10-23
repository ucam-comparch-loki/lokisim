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
  switch (state) {
    case WAITING_FOR_DATA : {
      // Data has now arrived.

      // Don't allow any more data until this send is complete.
      flowControlOut.write(false);

      // If the network is not ready for the data, wait until it is.
      if (!readyIn.read())
        next_trigger(readyIn.posedge_event());
      else
        handleNewData();

      break;
    }

    case WAITING_FOR_CREDITS : {
      // A credit has now arrived.
      sendData();

      state = WAITING_FOR_DATA;
      next_trigger(dataIn.default_event());

      break;
    }
  } // end switch
}

void FlowControlOut::sendData() {
  assert(creditCount > 0);

  if (DEBUG) cout << "Network sending " << dataIn.read().payload() << " from "
      << channel << " to " << dataIn.read().channelID() << endl;

  dataOut.write(dataIn.read());
  creditCount--;
  flowControlOut.write(true);
}

void FlowControlOut::handleNewData() {
  if (dataIn.read().portClaim()) {
    // This message is allowed to send even if we have no credits
    // because it is not buffered -- it is immediately consumed to store
    // the return address at the destination.

    if (DEBUG) cout << "Sending port claim from " << channel << " to "
                    << dataIn.read().channelID() << endl;
  }
  else {
    if (creditCount <= 0) {  // We are not able to send the new data yet.
      if (DEBUG) cout << "Can't send from " << channel << ": no credits." << endl;

      // Wait until we receive a credit.
      state = WAITING_FOR_CREDITS;
      next_trigger(creditsIn.default_event());
    }
    else {
      sendData();
      next_trigger(dataIn.default_event());
    }
  }
}

void FlowControlOut::receivedCredit() {
  creditCount++;

  if (DEBUG) cout << this->name() << " received credit at port " << channel << endl;
  assert(creditCount <= IN_CHANNEL_BUFFER_SIZE);
}

FlowControlOut::FlowControlOut(sc_module_name name, const ComponentID& ID, const ChannelID& channelManaged) :
    Component(name, ID) {

  channel = channelManaged;
  creditCount = IN_CHANNEL_BUFFER_SIZE;
  state = WAITING_FOR_DATA;

  SC_METHOD(mainLoop);
  sensitive << dataIn;
  dont_initialize();

  SC_METHOD(receivedCredit);
  sensitive << creditsIn;
  dont_initialize();

  // Start execution allowing all inputs.
  flowControlOut.initialize(true);
  readyOut.initialize(true);

  end_module(); // Needed because we're using a different Component constructor

}
