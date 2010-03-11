/*
 * SendChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "SendChannelEndTable.h"

/* Put the received Word into the table, along with its destination address. */
void SendChannelEndTable::receivedData() {
  AddressedWord w(input.read(), remoteChannel.read());
  int buff = chooseBuffer();

  if(!(buffers[buff].isFull())) {
    buffers.write(w, buff);
    if(DEBUG) cout << "Wrote " << w << " to output channel-end " << buff << endl;
  }
  else {
    // TODO: stall pipeline
    if(DEBUG) cout << "Wrote to full buffer in Send Channel-end Table." << endl;
  }
}

/* If it is possible to send data onto the network, do it */
void SendChannelEndTable::canSend() {

  // If a buffer has information, and is allowed to send, put it in the vector
  for(int i=0; i<NUM_SEND_CHANNELS; i++) {
    bool send = flowControl[i].read();

    if(!(buffers[i].isEmpty()) && send) {
      output[i].write(buffers.read(i));
    }
  }

}

/* Choose which Buffer to put new data into. All data going to the same
 * destination must go in the same Buffer to ensure that packets remain in
 * order. This is possibly the simplest implementation. */
short SendChannelEndTable::chooseBuffer() {
  return remoteChannel.read() % NUM_SEND_CHANNELS;
}


SendChannelEndTable::SendChannelEndTable(sc_module_name name) :
    Component(name),
    buffers(NUM_SEND_CHANNELS, CHANNEL_END_BUFFER_SIZE) {

  flowControl = new sc_in<bool>[NUM_SEND_CHANNELS];
  output      = new sc_out<AddressedWord>[NUM_SEND_CHANNELS];

  SC_METHOD(receivedData);
  sensitive << input;
  dont_initialize();

  SC_METHOD(canSend);
  /*for(int i=0; i<NUM_SEND_CHANNELS; i++)*/ sensitive << clock.pos();//flowControl[i];
  dont_initialize();

}

SendChannelEndTable::~SendChannelEndTable() {

}
