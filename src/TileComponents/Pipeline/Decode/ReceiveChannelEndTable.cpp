/*
 * ReceiveChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "ReceiveChannelEndTable.h"
#include "../IndirectRegisterFile.h"
#include "../../../Datatype/Word.h"
#include "../../../Exceptions/BlockedException.h"

typedef IndirectRegisterFile Registers;

#define NO_CHANNEL -1
#define ALL_CHANNELS -2

int32_t ReceiveChannelEndTable::read(ChannelIndex channelEnd) {

  if(!buffers[channelEnd].isEmpty()) {
    int32_t result = buffers.read(channelEnd).toInt();
    updateFlowControl(channelEnd);

    if(DEBUG) cout << this->name() << " read " << result << " from channel "
                   << channelEnd << endl;

    return result;
  }
  else {   // Reading from empty buffer
    throw BlockedException();
  }

}

/* Return whether or not the specified channel contains data. */
bool ReceiveChannelEndTable::testChannelEnd(ChannelIndex channelEnd) const {
  return buffers[channelEnd].isEmpty();
}

ChannelIndex ReceiveChannelEndTable::selectChannelEnd() {
  int current = currentChannel.value();

  // Check all of the channels in a round-robin style, using a LoopCounter.
  for(int i = ++currentChannel; i != current; ++currentChannel) {
    if(!buffers[i].isEmpty()) {
      // Adjust address so it can be accessed like a register
      return Registers::fromChannelID(i);
    }
  }

  // If we have got this far, all the buffers are empty
  throw BlockedException();

}

/* Put any newly received values into their respective buffers. */
void ReceiveChannelEndTable::checkInputs() {
  // Do something if we want more channel ends than channels?
  for(uint i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    if(fromNetwork[i].event()) {
      buffers[i].write(fromNetwork[i].read());
      updateFlowControl(i);

      if(DEBUG) cout << this->name() << " channel " << i << " received " <<
                        fromNetwork[i].read() << endl;
    }
  }
}

void ReceiveChannelEndTable::updateFlowControl(uint8_t channelEnd) {
  flowControl[channelEnd].write(buffers[channelEnd].remainingSpace());
}

ReceiveChannelEndTable::ReceiveChannelEndTable(sc_module_name name) :
    Component(name),
    buffers(NUM_RECEIVE_CHANNELS, CHANNEL_END_BUFFER_SIZE),
    currentChannel(NUM_RECEIVE_CHANNELS) {

  flowControl = new sc_out<int>[NUM_RECEIVE_CHANNELS];
  fromNetwork = new sc_in<Word>[NUM_RECEIVE_CHANNELS];

}

ReceiveChannelEndTable::~ReceiveChannelEndTable() {

}
