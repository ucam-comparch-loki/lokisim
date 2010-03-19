/*
 * ReceiveChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "ReceiveChannelEndTable.h"

#define NO_CHANNEL -1
#define ALL_CHANNELS -2

/* Put any newly received values into their respective buffers. */
void ReceiveChannelEndTable::receivedInput() {
  // Do something if we want more channel ends than channels?
  for(int i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    if(fromNetwork[i].event()) {
      buffers.write(fromNetwork[i].read(), i);
      wroteToBuffer.write(!wroteToBuffer.read());
      checkWaiting(i);  // See if we have been waiting for data to arrive here

      cout << this->name() << " received " << fromNetwork[i].read() << endl;
    }
  }
}

/* Read a value for the first ALU input. */
void ReceiveChannelEndTable::read1() {
  read(fromDecoder1.read(), 0);
}

/* Read a value for the second ALU input. */
void ReceiveChannelEndTable::read2() {
  read(fromDecoder2.read(), 1);
}

/* Read from the chosen channel end, and write the result to the given output. */
void ReceiveChannelEndTable::read(short inChannel, short outChannel) {

  Word w;

  if(!buffers[inChannel].isEmpty()) {
    w = buffers.read(inChannel);
    readFromBuffer.write(!readFromBuffer.read());
  }
  else {   // Reading from empty buffer

    // Stall until we have data
    stallValue = true;
    updateStall1.write(!updateStall1.read());

    // Need to remember that we're waiting for data from this channel
    if(outChannel==0) waiting1 = inChannel;
    else if(outChannel==1) waiting2 = inChannel;

    return;

  }

  Data d = static_cast<Data>(w);

  if(outChannel==0) {
    dataToALU1 = d;
    updateToALU1_1.write(!updateToALU1_1.read());
  }
  else if(outChannel==1) toALU2.write(d);

}

/* Carry out the operation asked of this component. */
void ReceiveChannelEndTable::doOperation() {
  switch(operation.read()) {

    // Return whether or not the given channel contains data
    case TSTCH : {
      int testedChannel = fromDecoder1.read();
      if(buffers[testedChannel].isEmpty()) dataToALU1 = Data(0);
      else dataToALU1 = Data(1);
      updateToALU1_2.write(!updateToALU1_2.read());
    }

    // Return the value of the first buffer with data in it (stall if none)
    case SELCH : {

      for(int i=0; i<NUM_RECEIVE_CHANNELS; i++) { // TODO: round-robin
        if(!buffers[i].isEmpty()) {
          dataToALU1 = Data(i);
          updateToALU1_2.write(!updateToALU1_2.read());
          return;
        }
      }

      // If we have got this far, all the buffers are empty
      waiting1 = ALL_CHANNELS;  // Is it safe to overwrite the existing value?
      stallValue = true;
      updateStall3.write(!updateStall3.read());

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

  if(waiting1 == ALL_CHANNELS) {
    // Send the index of the channel-end with data
    dataToALU1 = Data(channelEnd);
    updateToALU1_3.write(!updateToALU1_3.read());
    waiting1 = NO_CHANNEL;
  }
  else if(waiting1 == channelEnd) {
    read(waiting1, 0);
    waiting1 = NO_CHANNEL;  // Not waiting for anything anymore
  }
  if(waiting2 == channelEnd) {
    read(waiting2, 1);
    waiting2 = NO_CHANNEL;  // Not waiting for anything anymore
  }

  if(waiting1 == NO_CHANNEL && waiting2 == NO_CHANNEL) {
    stallValue = false;
    updateStall2.write(!updateStall2.read());
  }

}

/* Update the this component's stalling status. */
void ReceiveChannelEndTable::updateStall() {
  stallOut.write(stallValue);
}

ReceiveChannelEndTable::ReceiveChannelEndTable(sc_module_name name) :
    Component(name),
    buffers(NUM_RECEIVE_CHANNELS, CHANNEL_END_BUFFER_SIZE) {

  flowControl = new sc_out<bool>[NUM_RECEIVE_CHANNELS];
  fromNetwork = new sc_in<Word>[NUM_RECEIVE_CHANNELS];

  waiting1 = waiting2 = NO_CHANNEL;
  stallValue = false;

  SC_METHOD(receivedInput);
  for(int i=0; i<NUM_RECEIVE_CHANNELS; i++) sensitive << fromNetwork[i];
  dont_initialize();

  SC_METHOD(read1);
  sensitive << fromDecoder1;
  dont_initialize();

  SC_METHOD(read2);
  sensitive << fromDecoder2;
  dont_initialize();

  SC_METHOD(doOperation);
  sensitive << operation;
  dont_initialize();

  SC_METHOD(updateToALU1);
  sensitive << updateToALU1_1 << updateToALU1_2 << updateToALU1_3;
  dont_initialize();

  SC_METHOD(updateFlowControl);
  sensitive << readFromBuffer << wroteToBuffer;
  // do initialise

  SC_METHOD(updateStall);
  sensitive << updateStall1 << updateStall2;
  // do initialise

}

ReceiveChannelEndTable::~ReceiveChannelEndTable() {

}
