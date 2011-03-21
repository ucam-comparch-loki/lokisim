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
  if(dataIn.read().portClaim()) {
    // Set the return address so we can send flow control.
    returnAddress = dataIn.read().payload().toInt();
    numCredits++;
    assert(numCredits >= 0 && numCredits <= CHANNEL_END_BUFFER_SIZE);

    // Wake up the sendCredit thread.
    newCredit.notify(sc_core::SC_ZERO_TIME);

    if(DEBUG) cout << "Port " << TileComponent::inputPortString(id)
         << " was claimed by " << TileComponent::outputPortString(returnAddress)
         << endl;
  }
  else {
    // Pass the value to the component.
    dataOut.write(dataIn.read().payload());
  }
}

void FlowControlIn::receiveFlowControl() {
  while(true) {
    wait(creditsIn.default_event());
    numCredits++;
    assert(numCredits >= 0 && numCredits <= CHANNEL_END_BUFFER_SIZE);

    // Wake up the sendCredit thread.
    newCredit.notify(sc_core::SC_ZERO_TIME);
  }
}

void FlowControlIn::sendCredit() {
  while(true) {
    // Wait until our next credit arrives, unless we already have one waiting.
    if(numCredits>0) wait(1, sc_core::SC_NS);
    else wait(newCredit);

    // Wait until we are allowed to send the credit.
    if(!canSendCredits.read()) wait(canSendCredits.posedge_event());

    // Send the new credit if someone is communicating with this port.
    if((int)returnAddress != -1) {
      AddressedWord aw(Word(1), returnAddress);
      creditsOut.write(aw);
      numCredits--;
      assert(numCredits >= 0 && numCredits <= CHANNEL_END_BUFFER_SIZE);

    if(DEBUG) cout << "Sent credit from port "
         << TileComponent::inputPortString(id) << " to "
         << TileComponent::outputPortString(returnAddress) << endl;
    }
  }
}

FlowControlIn::FlowControlIn(sc_module_name name, ComponentID ID) :
    Component(name, ID) {

  returnAddress = -1;
  numCredits = 0;

  SC_METHOD(receivedData);
  sensitive << dataIn;
  dont_initialize();

  SC_THREAD(receiveFlowControl);
  SC_THREAD(sendCredit);

  canReceiveData.initialize(true);

  end_module(); // Needed because we're using a different Component constructor

}
