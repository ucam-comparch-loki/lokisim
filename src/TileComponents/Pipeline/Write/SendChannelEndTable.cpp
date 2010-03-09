/*
 * SendChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "SendChannelEndTable.h"

/* Put the received Word into the table, along with its destination address. */
void SendChannelEndTable::doOp() {
  AddressedWord *w = new AddressedWord(input.read(), remoteChannel.read());
  Buffer<AddressedWord>& b = buffers.at(chooseBuffer());

  if(!b.isFull()) {
    b.write(*w);
    if(DEBUG) std::cout<<"Wrote "<<*w<<" to output channel-end "<<chooseBuffer()<<std::endl;
  }
  else {
    // TODO: stall pipeline
    if(DEBUG) std::cout << "Wrote to full buffer in Send Channel-end Table." << std::endl;
  }
}

/* If it is possible to send data onto the network, do it */
void SendChannelEndTable::canSend() {

  // If a buffer has information, and is allowed to send, put it in the vector
  for(int i=0; i<NUM_SEND_CHANNELS; i++) {
    bool send = flowControl[i].read();

    if(!(buffers.at(i).isEmpty()) && send) {
      output[i].write(buffers.at(i).read());
    }
  }

}

/* Choose which Buffer to put new data into. All data going to the same
 * destination must go in the same Buffer to ensure that packets remain in
 * order. This is possibly the simplest implementation. */
short SendChannelEndTable::chooseBuffer() {
  return remoteChannel.read() % NUM_SEND_CHANNELS;
}


SendChannelEndTable::SendChannelEndTable(sc_core::sc_module_name name) :
    Component(name) {

  flowControl = new sc_in<bool>[NUM_SEND_CHANNELS];
  output = new sc_out<AddressedWord>[NUM_SEND_CHANNELS];

  for(int i=0; i<NUM_SEND_CHANNELS; i++) {
    Buffer<AddressedWord>* buffer = new Buffer<AddressedWord>(CHANNEL_END_BUFFER_SIZE);
    buffers.push_back(*buffer);
  }

  SC_METHOD(doOp);
  sensitive << input;
  dont_initialize();

  SC_METHOD(canSend);
  for(int i=0; i<NUM_SEND_CHANNELS; i++) sensitive << clock.pos();//flowControl[i];
  dont_initialize();

}

SendChannelEndTable::~SendChannelEndTable() {

}
