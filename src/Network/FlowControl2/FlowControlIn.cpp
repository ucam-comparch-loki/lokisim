/*
 * FlowControlIn.cpp
 *
 *  Created on: 8 Nov 2010
 *      Author: db434
 */

#include "FlowControlIn.h"
#include "../NetworkHierarchy.h"

/* Put any new data into the buffer. Since we approved the request to send
 * data, it is known that there is enough room. */
void FlowControlIn::receivedData() {
  if(dataIn.read().portClaim()) {
    // Set the return address so we can send flow control.
    returnAddress = dataIn.read().payload().toInt();

    if(DEBUG) cout << "Port " << NetworkHierarchy::portLocation(id, true)
         << " was claimed by " << NetworkHierarchy::portLocation(returnAddress, false)
         << endl;
  }
  else {
    // Pass the value to the component.
    dataOut.write(dataIn.read().payload());
  }
}

void FlowControlIn::receivedFlowControl() {
  // Send the new credit if someone is communicating with this port.
  if(readyIn.read() && (int)returnAddress != -1) {
    AddressedWord aw(Word(1), returnAddress);
    creditsOut.write(aw);

//    if(DEBUG) cout << "Sent credit to port "
//         << NetworkHierarchy::portLocation(returnAddress, false) << endl;
  }

  // With end-to-end flow control in place, we can always claim to be ready
  // to receive data, because we know the single source will not send too much.
  readyOut.write(true);

  if(!readyIn.read()) {
    // TODO: what do we do if readyIn is false? Seems silly to store a buffer
    // of credits.
    cerr << "Lost a credit" << endl;
  }
}

FlowControlIn::FlowControlIn(sc_module_name name, ComponentID ID) :
    Component(name, ID) {

  returnAddress = -1;

  SC_METHOD(receivedData);
  sensitive << dataIn;
  dont_initialize();

  SC_METHOD(receivedFlowControl);
  sensitive << flowControlIn;
  dont_initialize();

  readyOut.initialize(true);

  end_module(); // Needed because we're using a different Component constructor

}

FlowControlIn::~FlowControlIn() {

}
