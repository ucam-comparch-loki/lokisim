/*
 * FlowControlIn.cpp
 *
 *  Created on: 8 Nov 2010
 *      Author: db434
 */

#include "FlowControlIn.h"
#include "../../Datatype/AddressedWord.h"
#include "../../Datatype/MemoryRequest.h"
#include "../../TileComponents/TileComponent.h"

/* Put any new data into the buffer. Since we approved the request to send
 * data, it is known that there is enough room. */
void FlowControlIn::receivedData() {
  while(true) {
    // Wait for data to arrive.
    wait(validDataIn.posedge_event());

    if(dataIn.read().portClaim()) {
      // TODO: only accept the port claim when we have no credits left to send.

      // Set the return address so we can send flow control.
      returnAddress = dataIn.read().payload().toInt();
      numCredits++;
      assert(numCredits >= 0 && numCredits <= CHANNEL_END_BUFFER_SIZE);

      // Wake up the sendCredit thread.
      newCredit.notify();

      if(DEBUG) cout << "Channel " << channel.getString()
           << " was claimed by " << returnAddress.getString()
           << endl;
    }
    else {
      // Pass the value to the component.
      dataOut.write(dataIn.read().payload());
    }

    ackDataIn.write(true);
    wait(sc_core::SC_ZERO_TIME);
    ackDataIn.write(false);
  }
}

void FlowControlIn::receiveFlowControl() {
  while(true) {
    wait(creditsIn.default_event());
    numCredits++;
    assert(numCredits >= 0 && numCredits <= CHANNEL_END_BUFFER_SIZE);

    // Wake up the sendCredit thread.
    newCredit.notify();
  }
}

void FlowControlIn::sendCredit() {
  while(true) {
    // Wait until our next credit arrives, unless we already have one waiting.
    if(numCredits>0) wait(1, sc_core::SC_NS);
    else wait(newCredit);

    // Send the new credit if someone is communicating with this port.
    if(!returnAddress.isNullMapping()) {
      AddressedWord aw(Word(1), returnAddress);
      creditsOut.write(aw);
      validCreditOut.write(true);
      numCredits--;
      assert(numCredits >= 0 && numCredits <= CHANNEL_END_BUFFER_SIZE);

      if(DEBUG) cout << "Sent credit from "
         << channel.getString() << " to "
         << returnAddress.getString() << endl;

      // Deassert the valid signal when the acknowledgement arrives.
      wait(ackCreditOut.posedge_event());
      validCreditOut.write(false);
    }
  }
}

FlowControlIn::FlowControlIn(sc_module_name name, const ComponentID& ID, const ChannelID& channelManaged) :
    Component(name, ID) {

  channel = channelManaged;
  returnAddress = -1;
  numCredits = 0;

  SC_THREAD(receivedData);
  SC_THREAD(receiveFlowControl);
  SC_THREAD(sendCredit);

  end_module(); // Needed because we're using a different Component constructor

}
