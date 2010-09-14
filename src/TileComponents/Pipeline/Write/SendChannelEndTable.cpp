/*
 * SendChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "SendChannelEndTable.h"

void SendChannelEndTable::write(DecodedInst& dec) {
  if(dec.getOperation()==InstructionMap::SETCHMAP && dec.getImmediate() != 0) {
    updateMap(dec.getImmediate(), dec.getOperand1());
  }
  else if(dec.getOperation() == InstructionMap::WOCHE) {
    waitUntilEmpty(dec.getResult());  // is "result" right?
  }
  else if(dec.getChannelMap() != NULL_MAPPING) {
    Word w(dec.getResult());
    AddressedWord aw(w, getChannel(dec.getChannelMap()));
    uint8_t index = chooseBuffer(dec.getChannelMap());

    // We know it is safe to write to any buffer because we block the rest
    // of the pipeline if a buffer is full.
    buffers[index].write(aw);

    if(DEBUG) cout << this->name() << " wrote " << w
                   << " to output buffer " << index << endl;
  }
}

// A very simplistic implementation for now: we block any further data if
// even one buffer is full. Future implementations could allow data into
// other buffers.
bool SendChannelEndTable::isFull() {
  if(waitingOn != -1) return true;

  for(uint i=0; i<buffers.size(); i++) {
    if(buffers[i].isFull()) return true;
  }

  return false;
}

void SendChannelEndTable::send() {
  // If a buffer has information, and is allowed to send, send it
  for(uint i=0; i<buffers.size(); i++) {
    bool send = flowControl[i].read();

    if(!(buffers[i].isEmpty()) && send) {
      output[i].write(buffers[i].read());

      // See if we can stop waiting
      if(waitingOn == i && buffers[i].isEmpty()) {
        waitingOn = 255;
      }
    }
  }
}

/* Stall the pipeline until the specified channel is empty. */
void SendChannelEndTable::waitUntilEmpty(uint8_t channel) {

  if(!buffers[channel].isEmpty()) {
    waitingOn = channel;
    stallValue = true;
  }

}

/* Choose which Buffer to put new data into. All data going to the same
 * destination must go in the same Buffer to ensure that packets remain in
 * order. This is possibly the simplest implementation. */
uint8_t SendChannelEndTable::chooseBuffer(uint8_t channelMapEntry) {
  return channelMapEntry % NUM_SEND_CHANNELS;
}

/* Update an entry in the channel mapping table. */
void SendChannelEndTable::updateMap(uint8_t entry, uint32_t newVal) {
  // We subtract 1 from the entry position because entry 0 is used as the
  // fetch channel, and is stored elsewhere.
  channelMap.write(newVal, entry-1);
}

uint32_t SendChannelEndTable::getChannel(uint8_t mapEntry) {
  // We subtract 1 from the entry position because entry 0 is used as the
  // fetch channel, and is stored elsewhere.
  return channelMap.read(mapEntry-1);
}

SendChannelEndTable::SendChannelEndTable(sc_module_name name) :
    Component(name),
    buffers(NUM_SEND_CHANNELS, CHANNEL_END_BUFFER_SIZE),
    channelMap(CHANNEL_MAP_SIZE-1) {

  flowControl = new sc_in<bool>[NUM_SEND_CHANNELS];
  output      = new sc_out<AddressedWord>[NUM_SEND_CHANNELS];

  stallValue  = false;
  waitingOn   = -1;

  // Start with no mappings at all
  for(int i=0; i<channelMap.size(); i++) {
    updateMap(i, NULL_MAPPING);
  }
}

SendChannelEndTable::~SendChannelEndTable() {

}
