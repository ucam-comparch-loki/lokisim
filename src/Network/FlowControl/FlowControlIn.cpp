/*
 * FlowControlIn.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#include "FlowControlIn.h"

/* Put any new data into the buffers. Since we approved the request to send
 * data, it is known that there is enough room. */
void FlowControlIn::receivedData() {
  for(int i=0; i<width; i++) {
    // Only take action if an input value is new.
    if(dataIn[i].event()) {
      if(dataIn[i].read().portClaim()) {
        // Set the return address so we can send flow control.
        returnAddresses[i] = dataIn[i].read().payload().toInt();

        if(DEBUG) cout << this->name() << ": port " << i << " was claimed by "
                       << returnAddresses[i] << endl;

        // Send initial flow control data here if necessary.
      }
      else {
        // Pass the value to the component.
        dataOut[i].write(dataIn[i].read().payload());
      }
    }
  }
}

void FlowControlIn::receivedFlowControl() {
  for(int i=0; i<width; i++) {
    // Send the new credit if someone is communicating with this port.
    if(flowControl[i].event() && ((int)returnAddresses[i] != -1)) {
      AddressedWord aw(Word(1), returnAddresses[i]);
      credits[i].write(aw);
//      cout << this->name() << " sent credit from port " << i << " to " << returnAddresses[i] << endl;
    }

    bufferSpace[i] = flowControl[i].read();
  }
}

FlowControlIn::FlowControlIn(sc_module_name name, int width) :
    Component(name),
    width(width),
    flitsRemaining(width, 0),
    returnAddresses(width, -1),
    bufferSpace(width, 0) {

  dataIn      = new sc_in<AddressedWord>[width];
  flowControl = new sc_in<int>[width];
  dataOut     = new sc_out<Word>[width];
  credits     = new sc_out<AddressedWord>[width];

  SC_METHOD(receivedData);
  for(int i=0; i<width; i++) sensitive << dataIn[i];
  dont_initialize();

  SC_METHOD(receivedFlowControl);
  for(int i=0; i<width; i++) sensitive << flowControl[i];
  dont_initialize();

}

FlowControlIn::~FlowControlIn() {
  delete[] dataIn;
  delete[] dataOut;
  delete[] flowControl;
  delete[] credits;
}
