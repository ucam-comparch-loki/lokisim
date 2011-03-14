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
  // We know it is safe to write to any buffer because we block the rest
  // of the pipeline if a buffer is full.
  assert(!buffer.full());
  AddressedWord adjusted = data;
//  adjusted.channelID(output);
  buffer.write(adjusted);
//  channelMap[output].removeCredit();

  if(DEBUG) cout << this->name() << " wrote " << data.payload()
                 << " to output buffer" << endl;
}

/* Generate a memory request using the address from the ALU and the operation
 * supplied by the decoder. */
Word SendChannelEndTable::makeMemoryRequest(const DecodedInst& dec) const {
  MemoryRequest mr(dec.result(), dec.memoryOp());
  return mr;
}

bool SendChannelEndTable::full() const {
  return buffer.full();
}

void SendChannelEndTable::send() {
  // If a buffer has information, and is allowed to send, send it
  if(!(buffer.empty()) && flowControl.read()) {
    AddressedWord data = buffer.read();
//    MapIndex entry = data.channelID();
//    data.channelID(channelMap[entry].destination());

    output.write(data);
//    network.write(channelMap[entry].network());
  }
}

/* Stall the pipeline until the specified channel is empty. */
void SendChannelEndTable::waitUntilEmpty(MapIndex channel) {
  Instrumentation::stalled(id, true, Stalls::OUTPUT);

  // Whilst the chosen channel does not have all of its credits, wait until a
  // new credit is received.
  while(!channelMap[channel].haveAllCredits()) {
    wait(creditsIn.default_event());
  }

  Instrumentation::stalled(id, false);
}

/* Return a unique ID for an output port, so credits can be routed back to
 * it. This will become obsolete if/when we have static routing. */
ChannelID SendChannelEndTable::portID(ChannelIndex channel) const {
  return TileComponent::outputPortID(id, /*channel*/0);
}

/* Update an entry in the channel mapping table. */
void SendChannelEndTable::updateMap(MapIndex entry, ChannelID newVal) {
  assert(entry < CHANNEL_MAP_SIZE);
  assert(newVal < TOTAL_INPUTS);

  // All data sent to previous destinations must have been consumed before
  // we can change the destination.
  if(!channelMap[entry].haveAllCredits()) waitUntilEmpty(entry);

  channelMap[entry] = newVal;

  // Need to send port acquisition request out. FetchLogic sends out the claims
  // for entry 0, so we only do the others here.
  if(entry != 0) {
    AddressedWord aw(Word(portID(entry)), newVal, true);
    buffer.write(aw);
  }

  if(DEBUG) cout << this->name() << " updated map " << (int)entry << " to "
                 << newVal << endl;
}

ChannelID SendChannelEndTable::getChannel(MapIndex mapEntry) const {
  return channelMap[mapEntry].destination();
}

void SendChannelEndTable::receivedCredit() {
    // Note: this may not work if we have multiple tiles: it relies on all of the
    // addresses being neatly aligned.
    ChannelIndex targetCounter = creditsIn.read().channelID() % CHANNEL_MAP_SIZE;
    channelMap[targetCounter].addCredit();
}

SendChannelEndTable::SendChannelEndTable(sc_module_name name, ComponentID ID) :
    Component(name),
    buffer(CHANNEL_END_BUFFER_SIZE, string(name)),
    channelMap(CHANNEL_MAP_SIZE) {

  id = ID;

  SC_METHOD(receivedCredit);
  sensitive << creditsIn;
  dont_initialize();

}
