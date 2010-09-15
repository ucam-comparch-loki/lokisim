/*
 * SendChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "SendChannelEndTable.h"
#include "../../../Datatype/AddressedWord.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../../../Utility/InstructionMap.h"

void SendChannelEndTable::write(DecodedInst& dec) {

  if(dec.getOperation()==InstructionMap::SETCHMAP && dec.getImmediate() != 0) {
    updateMap(dec.getImmediate(), dec.getOperand1());
  }
  else if(dec.getOperation() == InstructionMap::WOCHE) {
    waitUntilEmpty(dec.getResult());  // is "result" right?
  }
  else if(dec.getChannelMap() != NULL_MAPPING) {
    Word w;

    if(dec.getMemoryOp() != MemoryRequest::NONE) {
      // Generate a special memory request if we are doing a load/store/etc.
      w = getMemoryRequest(dec);
    }
    else {
      w = Word(dec.getResult());
    }

    AddressedWord aw(w, getChannel(dec.getChannelMap()));
    ChannelIndex index = chooseBuffer(dec.getChannelMap());

    // We know it is safe to write to any buffer because we block the rest
    // of the pipeline if a buffer is full.
    buffers[index].write(aw);

    if(DEBUG) cout << this->name() << " wrote " << w
                   << " to output buffer " << index << endl;
  }

}

/* Generate a memory request using the address from the ALU and the operation
 * supplied by the decoder. */
Word SendChannelEndTable::getMemoryRequest(DecodedInst& dec) const {
  MemoryRequest mr(dec.getResult(), dec.getMemoryOp());
  return mr;
}

// A very simplistic implementation for now: we block any further data if
// even one buffer is full. Future implementations could allow data into
// other buffers.
bool SendChannelEndTable::isFull() {
  // We pretend to be full if we're waiting for a channel to empty.
  if(waitingOn != NO_CHANNEL) return true;

  // We say we are full if any one of our buffers are full.
  for(uint i=0; i<buffers.size(); i++) {
    if(buffers[i].isFull()) return true;
  }

  // If we have got this far, we are not waiting, and no buffers are full.
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
        waitingOn = NO_CHANNEL;
      }
    }
  }
}

/* Stall the pipeline until the specified channel is empty. */
void SendChannelEndTable::waitUntilEmpty(ChannelIndex channel) {
  if(!buffers[channel].isEmpty()) {
    waitingOn = channel;
  }
}

/* Choose which Buffer to put new data into. All data going to the same
 * destination must go in the same Buffer to ensure that packets remain in
 * order. This is possibly the simplest implementation. */
ChannelIndex SendChannelEndTable::chooseBuffer(MapIndex channelMapEntry) {
  return channelMapEntry % NUM_SEND_CHANNELS;
}

/* Update an entry in the channel mapping table. */
void SendChannelEndTable::updateMap(MapIndex entry, ChannelID newVal) {
  // We subtract 1 from the entry position because entry 0 is used as the
  // fetch channel, and is stored elsewhere.
  channelMap.write(newVal, entry-1);
}

ChannelID SendChannelEndTable::getChannel(MapIndex mapEntry) {
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

  waitingOn   = NO_CHANNEL;

  // Start with no mappings at all
  for(int i=0; i<channelMap.size(); i++) {
    updateMap(i, NULL_MAPPING);
  }
}

SendChannelEndTable::~SendChannelEndTable() {

}
