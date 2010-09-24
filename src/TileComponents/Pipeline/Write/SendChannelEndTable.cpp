/*
 * SendChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "SendChannelEndTable.h"
#include "../../../Datatype/AddressedWord.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/Instruction.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../../../Utility/InstructionMap.h"

const ChannelID SendChannelEndTable::NULL_MAPPING;

void SendChannelEndTable::write(DecodedInst& dec) {

  if(dec.operation()==InstructionMap::SETCHMAP) {
    updateMap(dec.immediate(), dec.result());
  }
  else if(dec.operation() == InstructionMap::WOCHE) {
    waitUntilEmpty(dec.result());
  }
  else if(dec.channelMapEntry() != Instruction::NO_CHANNEL &&
          getChannel(dec.channelMapEntry()) != NULL_MAPPING) {
    Word w;

    if(dec.memoryOp() != MemoryRequest::NONE) {
      // Generate a special memory request if we are doing a load/store/etc.
      w = makeMemoryRequest(dec);
    }
    else {
      w = Word(dec.result());
    }

    AddressedWord aw(w, getChannel(dec.channelMapEntry()));
    ChannelIndex index = chooseBuffer(dec.channelMapEntry());

    // We know it is safe to write to any buffer because we block the rest
    // of the pipeline if a buffer is full.
    buffers[index].write(aw);

    if(DEBUG) cout << this->name() << " wrote " << w
                   << " to output buffer " << (int)index << endl;
  }

}

/* Generate a memory request using the address from the ALU and the operation
 * supplied by the decoder. */
Word SendChannelEndTable::makeMemoryRequest(DecodedInst& dec) const {
  MemoryRequest mr(dec.result(), dec.memoryOp());
  return mr;
}

// A very simplistic implementation for now: we block any further data if
// even one buffer is full. Future implementations could allow data into
// other buffers.
bool SendChannelEndTable::isFull() const {
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
ChannelIndex SendChannelEndTable::chooseBuffer(MapIndex channelMapEntry) const {
  return channelMapEntry % NUM_SEND_CHANNELS;
}

/* Update an entry in the channel mapping table. */
void SendChannelEndTable::updateMap(MapIndex entry, ChannelID newVal) {
  channelMap.write(newVal, entry);

  if(DEBUG) cout << this->name() << " updated map " << (int)entry << " to "
                 << newVal << endl;
}

ChannelID SendChannelEndTable::getChannel(MapIndex mapEntry) {
  return channelMap.read(mapEntry);
}

SendChannelEndTable::SendChannelEndTable(sc_module_name name) :
    Component(name),
    buffers(NUM_SEND_CHANNELS, CHANNEL_END_BUFFER_SIZE),
    channelMap(CHANNEL_MAP_SIZE) {

  flowControl = new sc_in<bool>[NUM_SEND_CHANNELS];
  output      = new sc_out<AddressedWord>[NUM_SEND_CHANNELS];

  waitingOn   = NO_CHANNEL;

  // Start with no mappings at all.
  for(uint i=0; i<channelMap.size(); i++) {
    channelMap.write(NULL_MAPPING, i);
  }
}

SendChannelEndTable::~SendChannelEndTable() {

}
