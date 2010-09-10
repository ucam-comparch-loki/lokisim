/*
 * SendChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "SendChannelEndTable.h"

/* Put the received Word into the table, along with its destination address. */
void SendChannelEndTable::receivedData() {

//  if(operation == InstructionMap::SETCHMAP && immediate != 0) {
//    updateMap(immediate, operand1);
//  }

  AddressedWord w(input.read(), remoteChannel.read());

  // if(remoteChannel == NULL_MAPPING) return;

  int buff = chooseBuffer();

  if(!(buffers[buff].isFull())) {
    buffers.write(w, buff);

    // Stall so we don't receive any more data if the buffer is full
    if(buffers[buff].isFull()) {
      stallValue = true;
      wake(stallValueReady);
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
    wake(stallValueReady);
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
  wake(stallValueReady);

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

/* Update an entry in the channel mapping table. */
void SendChannelEndTable::updateMap(int entry, uint32_t newVal) {
  // We subtract 1 from the entry position because entry 0 is used as the
  // fetch channel, and is stored elsewhere.
  channelMap.write(newVal, entry-1);
}

SendChannelEndTable::SendChannelEndTable(sc_module_name name) :
    Component(name),
    buffers(NUM_SEND_CHANNELS, CHANNEL_END_BUFFER_SIZE),
    channelMap(CHANNEL_MAP_SIZE-1) {

  flowControl = new sc_in<bool>[NUM_SEND_CHANNELS];
  output      = new sc_out<AddressedWord>[NUM_SEND_CHANNELS];

  stallValue  = false;
  waiting     = false;

  // Start with no mappings at all
  for(int i=0; i<channelMap.size(); i++) {
    updateMap(i, NULL_MAPPING);
  }

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
  sensitive << stallValueReady;
  // do initialise;

}

SendChannelEndTable::~SendChannelEndTable() {

}
