/*
 * SendChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "SendChannelEndTable.h"
#include "../../TileComponent.h"
#include "../../../Datatype/AddressedWord.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/Instruction.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../../../Utility/InstructionMap.h"
#include "../../../Utility/Instrumentation/Stalls.h"

const ChannelID SendChannelEndTable::NULL_MAPPING;

void SendChannelEndTable::write(const DecodedInst& dec) {

  // Most messages we send will be one flit long, so will be the end of their
  // packets. However, packets for memory stores are two flits long (address
  // then data), so we need to mark this special case.
  bool endOfPacket = true;

  if(dec.operation() == InstructionMap::SETCHMAP) {
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

      if(dec.memoryOp() == MemoryRequest::STORE_W ||
         dec.memoryOp() == MemoryRequest::STORE_HW ||
         dec.memoryOp() == MemoryRequest::STORE_B) {
        endOfPacket = false;
      }
    }
    else {
      w = Word(dec.result());
    }

    AddressedWord aw(w, getChannel(dec.channelMapEntry()));
    if(!endOfPacket) aw.notEndOfPacket();

    write(aw, dec.channelMapEntry());
  }

}

void SendChannelEndTable::write(const AddressedWord& data, MapIndex output) {
  // There may be a different number of output ports to channel map entries.
  ChannelIndex index = chooseBuffer(output);

  // We know it is safe to write to any buffer because we block the rest
  // of the pipeline if a buffer is full.
  assert(!buffers[index].full());
  buffers[index].write(data);

  if(DEBUG) cout << this->name() << " wrote " << data.payload()
                 << " to output buffer " << (int)index << endl;
}

/* Generate a memory request using the address from the ALU and the operation
 * supplied by the decoder. */
Word SendChannelEndTable::makeMemoryRequest(const DecodedInst& dec) const {
  MemoryRequest mr(dec.result(), dec.memoryOp());
  return mr;
}

// A very simplistic implementation for now: we block any further data if
// even one buffer is full. Future implementations could allow data into
// other buffers.
bool SendChannelEndTable::full() const {
  // We pretend to be full if we're waiting for a channel to empty.
  if(waitingOn != NO_CHANNEL) return true;

  // We say we are full if any one of our buffers are full.
  for(uint i=0; i<buffers.size(); i++) {
    if(buffers[i].full()) return true;
  }

  // If we have got this far, we are not waiting, and no buffers are full.
  return false;
}

void SendChannelEndTable::send() {
  if(buffers.empty()) return;

  // If a buffer has information, and is allowed to send, send it
  for(uint i=0; i<buffers.size(); i++) {
    if(!(buffers[i].empty()) && flowControl[i].read()) {
      output[i].write(buffers[i].read());

      // See if we can stop waiting
      if(waitingOn == i && buffers[i].empty()) {
        waitingOn = NO_CHANNEL;
      }
    }
  }
}

/* Stall the pipeline until the specified channel is empty. */
void SendChannelEndTable::waitUntilEmpty(ChannelIndex channel) {
  // Whilst the buffer has data, wait until the buffer sends something,
  // then check again.
  Instrumentation::stalled(id, true, Stalls::OUTPUT);
  while(!buffers[channel].empty()) {
    wait(output[channel].default_event());
  }
  Instrumentation::stalled(id, false);
}

/* Choose which Buffer to put new data into. All data going to the same
 * destination must go in the same Buffer to ensure that packets remain in
 * order. This is possibly the simplest implementation. */
ChannelIndex SendChannelEndTable::chooseBuffer(MapIndex channelMapEntry) const {
  return channelMapEntry % NUM_SEND_CHANNELS;
}

/* Return a unique ID for an output port, so credits can be routed back to
 * it. This will become obsolete if/when we have static routing. */
ChannelID SendChannelEndTable::portID(ChannelIndex channel) const {
  return TileComponent::outputPortID(id, channel);
}

/* Update an entry in the channel mapping table. */
void SendChannelEndTable::updateMap(MapIndex entry, ChannelID newVal) {
  assert(entry < CHANNEL_MAP_SIZE);
  assert(newVal < TOTAL_INPUTS);
  channelMap.write(newVal, entry);

  // Need to send port acquisition request out. FetchLogic sends out the claims
  // for entry 0, so we only do the others here.
  if(entry != 0) {
    ChannelIndex index = chooseBuffer(entry);
    AddressedWord aw(Word(portID(index)), newVal, true);
    buffers[index].write(aw);
  }

  if(DEBUG) cout << this->name() << " updated map " << (int)entry << " to "
                 << newVal << endl;
}

ChannelID SendChannelEndTable::getChannel(MapIndex mapEntry) const {
  return channelMap.read(mapEntry);
}

SendChannelEndTable::SendChannelEndTable(sc_module_name name, ComponentID ID) :
    Component(name),
    buffers(NUM_SEND_CHANNELS, CHANNEL_END_BUFFER_SIZE, string(name)),
    channelMap(CHANNEL_MAP_SIZE, string(name)) {

  id = ID;

  flowControl = new sc_in<bool>[NUM_SEND_CHANNELS];
  output      = new sc_out<AddressedWord>[NUM_SEND_CHANNELS];

  waitingOn   = NO_CHANNEL;

  // Start with no mappings at all.
  for(uint i=0; i<channelMap.size(); i++) {
    channelMap.write(NULL_MAPPING, i);
  }
}

SendChannelEndTable::~SendChannelEndTable() {
  delete[] output;
  delete[] flowControl;
}
