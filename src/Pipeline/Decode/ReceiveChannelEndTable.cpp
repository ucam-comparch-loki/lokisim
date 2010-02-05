/*
 * ReceiveChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "ReceiveChannelEndTable.h"

void ReceiveChannelEndTable::receivedInput1() {
  // Do something if we have more than two channel ends (destination % numchannels)
  try {
    buffers.at(0).write(fromNetwork1.read());
  }
  catch(std::exception e) {
    // Drop packet and carry on
    //cout << "Dropped packet at receive channel end table " << id << endl;
  }
}

void ReceiveChannelEndTable::receivedInput2() {
  // Do something if we have more than two channel ends (destination % numchannels)
  try {
    buffers.at(1).write(fromNetwork2.read());
  }
  catch(std::exception e) {
    // Drop packet and carry on
    //cout << "Dropped packet at receive channel end table " << id << endl;
  }
}

void ReceiveChannelEndTable::read1() {
  read(fromDecoder1.read(), 0);
}

void ReceiveChannelEndTable::read2() {
  read(fromDecoder2.read(), 1);
}

/* Read from the chosen channel end, and write the result to the given output */
void ReceiveChannelEndTable::read(short inChannel, short outChannel) {

  Word w;

  try {
    w = buffers.at(inChannel).read();
  }
  catch(std::exception e) {   // Reading from empty buffer
    w = Word(0);              // Return 0 (should stall)
  }

  Data* d = new Data(w);

  if(outChannel==0) toALU1.write(*d);
  else if(outChannel==1) toALU2.write(*d);

}

ReceiveChannelEndTable::ReceiveChannelEndTable(sc_core::sc_module_name name, int ID)
    : Component(name, ID) {

  buffers = *(new vector<Buffer<Word> >(NUM_RECEIVE_CHANNELS));

  for(int i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    Buffer<Word>* buffer = new Buffer<Word>(CHANNEL_END_BUFFER_SIZE);
    buffers.at(i) = *buffer;
  }

  SC_METHOD(receivedInput1);
  sensitive << fromNetwork1;
  dont_initialize();

  SC_METHOD(receivedInput2);
  sensitive << fromNetwork2;
  dont_initialize();

  SC_METHOD(read1);
  sensitive << fromDecoder1;
  dont_initialize();

  SC_METHOD(read2);
  sensitive << fromDecoder2;
  dont_initialize();

}

ReceiveChannelEndTable::~ReceiveChannelEndTable() {

}
