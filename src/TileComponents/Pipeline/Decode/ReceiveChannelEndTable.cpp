/*
 * ReceiveChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "ReceiveChannelEndTable.h"
#include "../IndirectRegisterFile.h"

typedef IndirectRegisterFile Registers;

#define NO_CHANNEL -1
#define ALL_CHANNELS -2

/* Put any newly received values into their respective buffers. */
void ReceiveChannelEndTable::receivedInput() {
  // Do something if we want more channel ends than channels?
  for(int i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    if(fromNetwork[i].event()) {
      buffers.write(fromNetwork[i].read(), i);
      wake(contentsChanged);
      checkWaiting(i);  // See if we have been waiting for data to arrive here

      if(DEBUG) cout << this->name() << " channel " << i << " received " <<
                        fromNetwork[i].read() << endl;
    }
  }
}

/* Service requests for data once the channels to read from are known. */
void ReceiveChannelEndTable::receivedRequest() {

  // Read a value for the first ALU input, unless we just received an
  // operation to perform, in which case, the result of the operation should
  // be sent instead.
  if(fromDecoder1.event()) {
    if(waitingForInput) testChannelEnd();
    else read(fromDecoder1.read(), 0);
  }
  else if(endWaiting1.event()) {
    read(waiting1, 0);
    waiting1 = NO_CHANNEL;
  }

  // Read a value for the second ALU input.
  if(fromDecoder2.event()) {
    read(fromDecoder2.read(), 1);
  }
  else if(endWaiting2.event()) {
    read(waiting2, 1);
    waiting2 = NO_CHANNEL;
  }

  if(waiting1 == NO_CHANNEL && waiting2 == NO_CHANNEL) {
    stallValue = false;
    wake(stallValueReady);
  }

}

/* Read from the chosen channel end, and write the result to the given output. */
void ReceiveChannelEndTable::read(short inChannel, short outChannel) {

  Word w;

  if(!buffers[inChannel].isEmpty()) {
    w = buffers.read(inChannel);
    wake(contentsChanged);

    if(DEBUG) cout << this->name() << " read " << w << " from channel "
                   << inChannel << endl;
  }
  else {   // Reading from empty buffer

    // Stall until we have data
    stallValue = true;
    wake(stallValueReady);

    // Need to remember that we're waiting for data from this channel
    if(outChannel==0) waiting1 = inChannel;
    else if(outChannel==1) waiting2 = inChannel;

    return;

  }

  Data d = static_cast<Data>(w);

  if(outChannel==0) {
    dataToALU1 = d;
    wake(alu1ValueReady);
  }
  else if(outChannel==1) toALU2.write(d);

}

/* Return whether or not the specified channel contains data. */
void ReceiveChannelEndTable::testChannelEnd() {
  int testedChannel = fromDecoder1.read();
  bool empty = buffers[testedChannel].isEmpty();

  if(empty) dataToALU1 = Data(0);
  else dataToALU1 = Data(1);
  wake(alu1ValueReady);

  waitingForInput = false;
}

/* Carry out the operation asked of this component. The operation must arrive
 * at the same time as or after the input from fromDecoder1. */
void ReceiveChannelEndTable::doOperation() {
  switch(operation.read()) {

    // Wait until the channel ID to check arrives
    case TSTCH : {
      waitingForInput = true;
      break;
    }

    // Return the value of the first buffer with data in it (stall if none)
    case SELCH : {

      // Check all of the channels in a round-robin style, using a LoopCounter.
      int current = currentChannel.value();

      for(int i = ++currentChannel; i != current; ++currentChannel) {
        if(!buffers[i].isEmpty()) {
          // Adjust address so it can be accessed like a register
          dataToALU1 = Data(Registers::fromChannelID(i));
          wake(alu1ValueReady);
          return;
        }
      }

      // If we have got this far, all the buffers are empty
      waiting1 = ALL_CHANNELS;  // Is it safe to overwrite the existing value?
      stallValue = true;
      wake(stallValueReady);

      break;

    }

  }
}

/* Send new data out on the toALU1 port. */
void ReceiveChannelEndTable::updateToALU1() {
  toALU1.write(dataToALU1);
}

/* Update the flow control values when a buffer has been read from or
 * written to. */
void ReceiveChannelEndTable::updateFlowControl() {
  for(int i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    flowControl[i].write(!buffers[i].isFull());
  }
}

/* Check to see if we were waiting for data on this channel end, and if so,
 * send it immediately. Unstall the pipeline if appropriate. */
void ReceiveChannelEndTable::checkWaiting(int channelEnd) {

  if(waiting1 == ALL_CHANNELS) { // Send the index of the channel-end with data
    // Adjust address so it can be accessed like a register
    dataToALU1 = Data(Registers::fromChannelID(channelEnd));
    wake(alu1ValueReady);
    waiting1 = NO_CHANNEL;
  }
  else if(waiting1 == channelEnd) {
//    read(waiting1, 0);
//    waiting1 = NO_CHANNEL;  // Not waiting for anything anymore
    endWaiting1.write(!endWaiting1.read());
  }
  if(waiting2 == channelEnd) {
//    read(waiting2, 1);
//    waiting2 = NO_CHANNEL;  // Not waiting for anything anymore
    endWaiting2.write(!endWaiting2.read());
  }

  if(waiting1 == NO_CHANNEL && waiting2 == NO_CHANNEL) {
    stallValue = false;
    wake(stallValueReady);
  }

}

/* Update the this component's stalling status. */
void ReceiveChannelEndTable::updateStall() {
  stallOut.write(stallValue);
}

ReceiveChannelEndTable::ReceiveChannelEndTable(sc_module_name name) :
    Component(name),
    buffers(NUM_RECEIVE_CHANNELS, CHANNEL_END_BUFFER_SIZE),
    currentChannel(NUM_RECEIVE_CHANNELS) {

  flowControl = new sc_out<bool>[NUM_RECEIVE_CHANNELS];
  fromNetwork = new sc_in<Word>[NUM_RECEIVE_CHANNELS];

  waiting1 = waiting2 = NO_CHANNEL;
  stallValue = false;

  SC_METHOD(receivedInput);
  for(int i=0; i<NUM_RECEIVE_CHANNELS; i++) sensitive << fromNetwork[i];
  dont_initialize();

  SC_METHOD(receivedRequest);
  sensitive << fromDecoder1 << fromDecoder2 << endWaiting1 << endWaiting2;
  dont_initialize();

  SC_METHOD(doOperation);
  sensitive << operation;
  dont_initialize();

  SC_METHOD(updateToALU1);
  sensitive << alu1ValueReady;
  dont_initialize();

  SC_METHOD(updateFlowControl);
  sensitive << contentsChanged;
  // do initialise

  SC_METHOD(updateStall);
  sensitive << stallValueReady;
  // do initialise

}

ReceiveChannelEndTable::~ReceiveChannelEndTable() {

}
