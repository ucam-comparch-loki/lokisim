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

  // If there is no data, block until it arrives.
  if(buffers[channelEnd].empty()) {
    Instrumentation::stalled(id, true, Stalls::INPUT);
    wait(fromNetwork[channelEnd].default_event());
    wait(sc_core::SC_ZERO_TIME);  // Allow the new value to be put in a buffer.
    Instrumentation::stalled(id, false);
  }

  int32_t result = buffers.read(channelEnd).toInt();
  bufferEvent[channelEnd].notify();

  if(DEBUG) cout << this->name() << " read " << result << " from channel "
                 << (int)channelEnd << endl;

  return result;

}

/* Return whether or not the specified channel contains data. */
bool ReceiveChannelEndTable::testChannelEnd(ChannelIndex channelEnd) const {
  assert(channelEnd < NUM_RECEIVE_CHANNELS);
  return !buffers[channelEnd].empty();
}

ChannelIndex ReceiveChannelEndTable::selectChannelEnd() {
  int current = currentChannel.value();

  // Check all of the channels in a round-robin style, using a LoopCounter.
  for(int i = ++currentChannel; i != current; ++currentChannel) {
    if(!buffers[i].empty()) {
      // Adjust address so it can be accessed like a register
      return Registers::fromChannelID(i);
    }
  }

  // If we have got this far, all the buffers are empty
  throw BlockedException("receive channel-end table", id);

}

void ReceiveChannelEndTable::checkInput(ChannelIndex input) {
  // This method is called because data has arrived on a particular input channel.
  assert(!buffers[input].full());

  buffers[input].write(fromNetwork[input].read());
  bufferEvent[input].notify();

  if(DEBUG) cout << this->name() << " channel " << (int)input << " received " <<
                    fromNetwork[input].read() << endl;
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

ReceiveChannelEndTable::ReceiveChannelEndTable(sc_module_name name, const ComponentID& ID) :
    Component(name, ID),
    buffers(NUM_RECEIVE_CHANNELS, CHANNEL_END_BUFFER_SIZE, string(name)),
    currentChannel(NUM_RECEIVE_CHANNELS) {

  flowControl = new sc_out<bool>[NUM_RECEIVE_CHANNELS];
  fromNetwork = new sc_in<Word>[NUM_RECEIVE_CHANNELS];

  bufferEvent = new sc_core::sc_event[NUM_RECEIVE_CHANNELS];

  // Generate a method to watch each input port, putting the data into the
  // appropriate buffer when it arrives.
  for(unsigned int i=0; i<buffers.size(); i++) {
    sc_core::sc_spawn_options options;
    options.spawn_method();     // Want an efficient method, not a thread
    options.dont_initialize();  // Only execute when triggered
    options.set_sensitivity(&(fromNetwork[i])); // Sensitive to this port

    // Create the method.
    sc_spawn(sc_bind(&ReceiveChannelEndTable::checkInput, this, i), 0, &options);
  }

  // Generate a method to watch each buffer, updating its flow control signal
  // whenever data is added or removed.
  for(unsigned int i=0; i<buffers.size(); i++) {
    sc_core::sc_spawn_options options;
    options.spawn_method();     // Want an efficient method, not a thread
    options.set_sensitivity(&(bufferEvent[i])); // Sensitive to this event

    // Create the method.
    sc_spawn(sc_bind(&ReceiveChannelEndTable::updateFlowControl, this, i), 0, &options);
  }

}

ReceiveChannelEndTable::~ReceiveChannelEndTable() {
  delete[] flowControl;
  delete[] fromNetwork;

  delete[] bufferEvent;
}
