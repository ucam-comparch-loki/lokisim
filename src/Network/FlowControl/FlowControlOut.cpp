/*
 * FlowControlOut.cpp
 *
 *  Created on: 8 Nov 2010
 *      Author: db434
 */

#include "FlowControlOut.h"

#include "../../Utility/Assert.h"

void FlowControlOut::mainLoop() {
  switch (state) {
    case WAITING_FOR_DATA : {
      // Data has now arrived.

      // Don't allow any more data until this send is complete.
      oFlowControl.write(false);

      // If the network is not ready for the data, wait until it is.
      if (!iReady.read())
        next_trigger(iReady.posedge_event());
      else
        handleNewData();

      break;
    }

    case WAITING_FOR_CREDITS : {
      // A credit has now arrived.
      sendData();

      state = WAITING_FOR_DATA;
      next_trigger(iData.default_event());

      break;
    }
  } // end switch
}

void FlowControlOut::sendData() {
  loki_assert(creditCount > 0);

  LOKI_LOG << "Network sending " << iData.read().payload() << " from "
      << channel << " to " << iData.read().channelID() << endl;

  oData.write(iData.read());
  creditCount--;
  oFlowControl.write(true);
}

void FlowControlOut::handleNewData() {
//  if (iData.read().portClaim()) {
//    // This message is allowed to send even if we have no credits
//    // because it is not buffered -- it is immediately consumed to store
//    // the return address at the destination.
//
//    if (DEBUG) cout << "Sending port claim from " << channel << " to "
//                    << iData.read().channelID() << endl;
//  }
//  else {
//    if (creditCount <= 0) {  // We are not able to send the new data yet.
//      if (DEBUG) cout << "Can't send from " << channel << ": no credits." << endl;
//
//      // Wait until we receive a credit.
//      state = WAITING_FOR_CREDITS;
//      next_trigger(iCredit.default_event());
//    }
//    else {
//      sendData();
//      next_trigger(iData.default_event());
//    }
//  }
}

void FlowControlOut::receivedCredit() {
  creditCount++;

  LOKI_LOG << this->name() << " received credit at port " << channel << endl;
  loki_assert_with_message(creditCount <= maxCredits, "Credits = %d", creditCount);
}

FlowControlOut::FlowControlOut(sc_module_name name,
                               const ChannelID& channelManaged,
                               size_t maxCredits) :
    LokiComponent(name),
    iData("iData"),
    iCredit("iCredit"),
    oData("oData"),
    iReady("iReady"),
    oReady("oReady"),
    oFlowControl("oFlowControl"),
    maxCredits(maxCredits) {

  channel = channelManaged;
  creditCount = maxCredits;
  state = WAITING_FOR_DATA;

  SC_METHOD(mainLoop);
  sensitive << iData;
  dont_initialize();

  SC_METHOD(receivedCredit);
  sensitive << iCredit;
  dont_initialize();

  // Start execution allowing all inputs.
  oFlowControl.initialize(true);
  oReady.initialize(true);

  end_module(); // Needed because we're using a different Component constructor

}
