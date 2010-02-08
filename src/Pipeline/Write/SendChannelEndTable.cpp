/*
 * SendChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "SendChannelEndTable.h"

/* Put the received Word into the table, along with its destination address. */
void SendChannelEndTable::doOp() {
  AddressedWord *w = new AddressedWord(input.read(), 0, remoteChannel.read());
  buffers.at(chooseBuffer()).write(*w);
}

/* If it is possible to send data onto the network, do it */
void SendChannelEndTable::canSend() {
  Array<bool> flowCont = flowControl.read();    // Remove copy if possible

  // If a buffer has information, and is allowed to send, put it in the vector
  for(int i=0; i<NUM_SEND_CHANNELS; i++) {
    bool send = flowCont.get(i);
    if(!(buffers.at(i).isEmpty()) && send) {
      toSend.put(i, buffers.at(i).read());
    }
    // Otherwise, send the same value again (no change => no event)
  }

  output.write(toSend);

}

/* Choose which Buffer to put new data into. All data going to the same
 * destination must go in the same Buffer to ensure that packets remain in
 * order. This is possibly the simplest implementation. */
short SendChannelEndTable::chooseBuffer() {
  return remoteChannel.read() % NUM_SEND_CHANNELS;
}


SendChannelEndTable::SendChannelEndTable(sc_core::sc_module_name name, int ID) :
    Component(name, ID),
    buffers(NUM_SEND_CHANNELS),
    toSend(NUM_SEND_CHANNELS) {

  for(int i=0; i<NUM_SEND_CHANNELS; i++) {
    Buffer<AddressedWord>* buffer = new Buffer<AddressedWord>(CHANNEL_END_BUFFER_SIZE);
    buffers.at(i) = *buffer;
  }

  SC_METHOD(doOp);
  sensitive << input;
  dont_initialize();

  SC_METHOD(canSend);
  sensitive << flowControl;
  //dont_initialize();

}

SendChannelEndTable::~SendChannelEndTable() {

}
