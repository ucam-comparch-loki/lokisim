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

    // Stall so we don't receive any more data if the buffer is full
    if(buffers[buff].isFull()) {
      stallValue = true;
      updateStall1.write(!updateStall1.read());
    }

    if(DEBUG) cout<<this->name()<<" wrote "<<w<<" to output channel-end "<<buff<<endl;
  }
  else {
    if(DEBUG) cout << "Wrote to full buffer in Send Channel-end Table." << endl;
  }
}

/* Stall the pipeline until the specified channel is empty. */
void SendChannelEndTable::waitUntilEmpty() {

  int channel = waitOnChannel.read();

  if(!buffers[channel].isEmpty()) {
    waiting = true;
    stallValue = true;
    updateStall3.write(!updateStall3.read());
  }

}

/* If it is possible to send data onto the network, do it. */
void SendChannelEndTable::canSend() {

  bool stall = false;

  // If a buffer has information, and is allowed to send, put it in the vector
  for(int i=0; i<NUM_SEND_CHANNELS; i++) {
    bool send = flowControl[i].read();

    if(!(buffers[i].isEmpty()) && send) {
      output[i].write(buffers.read(i));

      // See if we can stop waiting
      if(waiting && waitOnChannel.read() == i && buffers[i].isEmpty()) {
        waiting = false;
      }
    }

    if(buffers[i].isFull()) stall = true;
  }

  stallValue = stall || waiting;
  updateStall2.write(!updateStall2.read());

}

/* Choose which Buffer to put new data into. All data going to the same
 * destination must go in the same Buffer to ensure that packets remain in
 * order. This is possibly the simplest implementation. */
short SendChannelEndTable::chooseBuffer() {
  return remoteChannel.read() % NUM_SEND_CHANNELS;
}

/* Update the value on the stallOut port. */
void SendChannelEndTable::updateStall() {
  stallOut.write(stallValue);
}

SendChannelEndTable::SendChannelEndTable(sc_module_name name) :
    Component(name),
    buffers(NUM_SEND_CHANNELS, CHANNEL_END_BUFFER_SIZE) {

  flowControl = new sc_in<bool>[NUM_SEND_CHANNELS];
  output      = new sc_out<AddressedWord>[NUM_SEND_CHANNELS];

  stallValue  = false;
  waiting     = false;

  SC_METHOD(receivedData);
  sensitive << input;
  dont_initialize();

  SC_METHOD(waitUntilEmpty);
  sensitive << waitOnChannel;
  dont_initialize();

  SC_METHOD(canSend);
  sensitive << clock.pos();
  dont_initialize();

  SC_METHOD(updateStall);
  sensitive << updateStall1 << updateStall2 << updateStall3;
  // do initialise;

}

SendChannelEndTable::~SendChannelEndTable() {

}
