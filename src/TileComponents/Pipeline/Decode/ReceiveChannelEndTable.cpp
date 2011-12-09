/*
 * ReceiveChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "ReceiveChannelEndTable.h"
#include "DecodeStage.h"
#include "../IndirectRegisterFile.h"
#include "../../../Datatype/Word.h"
#include "../../../Exceptions/BlockedException.h"
#include "../../../Utility/Instrumentation/Stalls.h"

typedef IndirectRegisterFile Registers;

int32_t ReceiveChannelEndTable::read(ChannelIndex channelEnd) {
  assert(channelEnd < NUM_RECEIVE_CHANNELS);
  assert(!buffers[channelEnd].empty());

  int32_t result = buffers.read(channelEnd).toInt();
  bufferEvent[channelEnd].notify();

  if(DEBUG) cout << this->name() << " read " << result << " from buffer "
                 << (int)channelEnd << endl;

  return result;

}

/* Return whether or not the specified channel contains data. */
bool ReceiveChannelEndTable::testChannelEnd(ChannelIndex channelEnd) const {
  assert(channelEnd < NUM_RECEIVE_CHANNELS);
  return !buffers[channelEnd].empty();
}

ChannelIndex ReceiveChannelEndTable::selectChannelEnd() {
  int startPoint = currentChannel.value();

  if(buffers.empty())
    wait(newData);

  // Check all of the channels in a round-robin style, using a LoopCounter.
  // Return the register-mapping of the first channel which has data.
  for(int i = ++currentChannel; i != startPoint; ++currentChannel) {
    i = currentChannel.value();

    if(!buffers[i].empty()) {
      // Adjust address so it can be accessed like a register
      return Registers::fromChannelID(i);
    }
  }

  // We should always find something in one of the buffers, because we wait
  // until at least one of them has data.
  assert(false);

}

void ReceiveChannelEndTable::checkInput(ChannelIndex input) {
  // This method is called because data has arrived on a particular input channel.

  if(DEBUG) cout << this->name() << " channel " << (int)input << " received " <<
                    fromNetwork[input].read() << endl;

  assert(!buffers[input].full());

  buffers[input].write(fromNetwork[input].read());
  bufferEvent[input].notify();
  newData.notify();
}

sc_core::sc_event& ReceiveChannelEndTable::receivedDataEvent(ChannelIndex buffer) const {
  return bufferEvent[buffer];
}

void ReceiveChannelEndTable::updateFlowControl(ChannelIndex buffer) {
  // Only update flow control information on the negative clock edge. (Why?)
  if(!clock.negedge()) next_trigger(clock.negedge_event());
  else {
    flowControl[buffer].write(!buffers[buffer].full());
    next_trigger(bufferEvent[buffer]);
  }
}

DecodeStage* ReceiveChannelEndTable::parent() const {
  return static_cast<DecodeStage*>(this->get_parent());
}

ReceiveChannelEndTable::ReceiveChannelEndTable(const sc_module_name& name, const ComponentID& ID) :
    Component(name, ID),
    buffers(NUM_RECEIVE_CHANNELS, IN_CHANNEL_BUFFER_SIZE, string(name)),
    currentChannel(NUM_RECEIVE_CHANNELS) {

  flowControl = new sc_out<bool>[NUM_RECEIVE_CHANNELS];
  fromNetwork = new sc_in<Word>[NUM_RECEIVE_CHANNELS];

  bufferEvent = new sc_core::sc_event[NUM_RECEIVE_CHANNELS];

  // Generate a method to watch each input port, putting the data into the
  // appropriate buffer when it arrives.
  for(unsigned int i=0; i<buffers.size(); i++)
    SPAWN_METHOD(fromNetwork[i], ReceiveChannelEndTable::checkInput, i, false);

  // Generate a method to watch each buffer, updating its flow control signal
  // whenever data is added or removed.
  for(unsigned int i=0; i<buffers.size(); i++)
    SPAWN_METHOD(bufferEvent[i], ReceiveChannelEndTable::updateFlowControl, i, true);

}

ReceiveChannelEndTable::~ReceiveChannelEndTable() {
  delete[] flowControl;
  delete[] fromNetwork;

  delete[] bufferEvent;
}
