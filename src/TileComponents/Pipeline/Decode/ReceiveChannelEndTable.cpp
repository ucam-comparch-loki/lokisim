/*
 * ReceiveChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

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

/* Put any newly received values into their respective buffers. */
void ReceiveChannelEndTable::checkInputs() {
  // We only send credits when there are spaces in the buffers, so we know it
  // is safe to write data to them now.
  for(uint i=0; i<buffers.size(); i++) {
    if(fromNetwork[i].event()) {
      assert(!buffers[i].full());

      buffers[i].write(fromNetwork[i].read());

      if(DEBUG) cout << this->name() << " channel " << i << " received " <<
                        fromNetwork[i].read() << endl;
    }
  }
}

void ReceiveChannelEndTable::updateFlowControl() {
  for(unsigned int i=0; i<buffers.size(); i++) {
    flowControl[i].write(!buffers[i].full());
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

  SC_METHOD(checkInputs);
  for(uint i=0; i<buffers.size(); i++) sensitive << fromNetwork[i];
  dont_initialize();

  SC_METHOD(updateFlowControl);
  sensitive << clock.neg();
  // do initialise

}

ReceiveChannelEndTable::~ReceiveChannelEndTable() {
  delete[] flowControl;
  delete[] fromNetwork;
}
