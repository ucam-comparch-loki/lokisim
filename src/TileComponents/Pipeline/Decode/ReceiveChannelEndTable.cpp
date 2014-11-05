/*
 * ReceiveChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "ReceiveChannelEndTable.h"
#include "DecodeStage.h"
#include "../RegisterFile.h"
#include "../../../Datatype/Word.h"
#include "../../../Exceptions/BlockedException.h"
#include "../../../Utility/Instrumentation/Stalls.h"

typedef RegisterFile Registers;

int32_t ReceiveChannelEndTable::read(ChannelIndex channelEnd) {
  assert(channelEnd < NUM_RECEIVE_CHANNELS);
  assert(!buffers[channelEnd].empty());

  // This path can only be followed from SC_THREADs. SC_METHODs must check
  // whether there is data in the buffer before reading from it.
//  if(buffers[channelEnd].empty())
//    wait(receivedDataEvent(channelEnd));

  int32_t result = buffers[channelEnd].read().toInt();

  if (DEBUG) cout << this->name() << " read " << result << " from buffer "
                  << (int)channelEnd << endl;

  return result;
}

int32_t ReceiveChannelEndTable::readDebug(ChannelIndex channelEnd) const {
  assert(channelEnd < NUM_RECEIVE_CHANNELS);

  if (buffers[channelEnd].empty())
    return 0;
  else
    return buffers[channelEnd].peek().toInt();
}

/* Return whether or not the specified channel contains data. */
bool ReceiveChannelEndTable::testChannelEnd(ChannelIndex channelEnd) const {
  assert(channelEnd < NUM_RECEIVE_CHANNELS);
  return !buffers[channelEnd].empty();
}

ChannelIndex ReceiveChannelEndTable::selectChannelEnd(unsigned int bitmask, const DecodedInst& inst) {
  // Wait for data to arrive on one of the channels we're interested in.
  waitForData(bitmask, inst);

  int startPoint = currentChannel.value();

  // Check all of the channels in a round-robin style, using a LoopCounter.
  // Return the register-mapping of the first channel which has data.
  for (int i = ++currentChannel; i != startPoint; ++currentChannel) {
    i = currentChannel.value();
    if (((bitmask >> i) & 1) && !buffers[i].empty()) {
        // Adjust address so it can be accessed like a register
        return Registers::fromChannelID(i);
    }
  }

  // We should always find something in one of the buffers, because we wait
  // until at least one of them has data.
  assert(false);
  return -1;
}

void ReceiveChannelEndTable::waitForData(unsigned int bitmask, const DecodedInst& inst) {
  // Wait for data to arrive on one of the channels we're interested in.
  while (true) {
    for (ChannelIndex i=0; i<NUM_RECEIVE_CHANNELS; i++) {
      if (((bitmask >> i) & 1) && !buffers[i].empty())
        return;
    }

    // Since multiple channels are involved, assume data can come from anywhere.
    Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_MEMORY_DATA, inst);
    Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_CORE_DATA, inst);
    wait(newData);
    Instrumentation::Stalls::unstall(id, Instrumentation::Stalls::STALL_MEMORY_DATA, inst);
    Instrumentation::Stalls::unstall(id, Instrumentation::Stalls::STALL_CORE_DATA, inst);
  }
}

void ReceiveChannelEndTable::checkInput(ChannelIndex input) {
  // This method is called because data has arrived on a particular input channel.

  if (DEBUG) cout << this->name() << " channel " << (int)input << " received " <<
                     iData[input].read() << endl;

  assert(!buffers[input].full());

  buffers[input].write(iData[input].read());

  newData.notify();
}

const sc_event& ReceiveChannelEndTable::receivedDataEvent(ChannelIndex buffer) const {
  return buffers[buffer].writeEvent();
}

void ReceiveChannelEndTable::updateFlowControl(ChannelIndex buffer) {
  bool canReceive = !buffers[buffer].full();
  if (oFlowControl[buffer].read() != canReceive)
    oFlowControl[buffer].write(canReceive);
}

void ReceiveChannelEndTable::dataConsumedAction(ChannelIndex buffer) {
  oDataConsumed[buffer].write(!oDataConsumed[buffer].read());
}

DecodeStage* ReceiveChannelEndTable::parent() const {
  return static_cast<DecodeStage*>(this->get_parent_object());
}

void ReceiveChannelEndTable::reportStalls(ostream& os) {
  for (uint i=0; i<buffers.size(); i++) {
    if (buffers[i].full())
      os << buffers[i].name() << " is full." << endl;
  }
}

ReceiveChannelEndTable::ReceiveChannelEndTable(const sc_module_name& name, const ComponentID& ID) :
    Component(name, ID),
    Blocking(),
    buffers(NUM_RECEIVE_CHANNELS, IN_CHANNEL_BUFFER_SIZE, this->name()),
    currentChannel(NUM_RECEIVE_CHANNELS) {

  iData.init(NUM_RECEIVE_CHANNELS);
  oFlowControl.init(NUM_RECEIVE_CHANNELS);
  oDataConsumed.init(NUM_RECEIVE_CHANNELS);

  // Generate a method to watch each input port, putting the data into the
  // appropriate buffer when it arrives.
  for (unsigned int i=0; i<buffers.size(); i++) {
    SPAWN_METHOD(iData[i], ReceiveChannelEndTable::checkInput, i, false);
    SPAWN_METHOD(buffers[i].dataConsumedEvent(), ReceiveChannelEndTable::dataConsumedAction, i, false);
  }

  // Generate a method to watch each buffer, updating its flow control signal
  // whenever data is added or removed.
  for (unsigned int i=0; i<buffers.size(); i++) {
    sc_core::sc_spawn_options options;
    options.spawn_method();     /* Want an efficient method, not a thread */
    options.set_sensitivity(&(buffers[i].readEvent()));
    options.set_sensitivity(&(buffers[i].writeEvent()));

    /* Create the method. */
    sc_spawn(sc_bind(&ReceiveChannelEndTable::updateFlowControl, this, i), 0, &options);
  }

}
