/*
 * MemoryMat.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "MemoryMat.h"
#include "../Datatype/Instruction.h"
#include "../Datatype/MemoryRequest.h"
#include "../Datatype/ChannelRequest.h"
#include "../Datatype/Data.h"

const int MemoryMat::NO_CONNECTION = -1;
const int MemoryMat::CONTROL_INPUT = NUM_CLUSTER_INPUTS - 1;

/* Look through all inputs for new data. Determine whether this data is the
 * start of a new transaction or the continuation of an existing one. Then
 * carry out the first/next step of the transaction. */
void MemoryMat::doOp() {

  // Inputs are always allowed to the control port
  flowControlOut[CONTROL_INPUT].write(true);

  if(in[CONTROL_INPUT].event()) updateControl();

  for(int i=0; i<NUM_CLUSTER_INPUTS-1; i++) {

    // Don't allow an input if there is no connection set up
    if(connections[i] == NO_CONNECTION) {
      flowControlOut[i].write(false);
      continue;
    }

    if(in[i].event()) {
      // If there isn't an existing transaction, this must be a read
      if(storeAddresses[i] == -1) {
        if(DEBUG) cout << "Received new memory request at input " << i << endl;

        MemoryRequest r = static_cast<MemoryRequest>(in[i].read());
        if(r.isReadRequest()) {
          if(r.isIPKRequest()) {
            readingIPK[i] = true;
            readAddresses[i] = r.getMemoryAddress();
          }
          read(i);
        }
        // Dealing with a write request
        else storeAddresses[i] = r.getMemoryAddress();
      }
      // If there is an existing transaction, this must be data to be written
      else write(in[i].read(), i);
    }
    // If there is a read transaction in progress, send the next value
    else if(readingIPK[i]) {
      read(i);
    }
    // No new input and no existing transaction => accept new transactions
    else {
      flowControlOut[i].write(true);
    }

  }

}

/* Carry out a read for the transaction at input "position". */
void MemoryMat::read(int position) {

  if(flowControlIn[position].read()) {

    Word w;
    int addr;

    if(readingIPK[position]) {
      addr = readAddresses[position];
      w = data.read(addr);

      if(static_cast<Instruction>(w).endOfPacket()) {
        readingIPK[position] = false;
        flowControlOut[position] = true;
      }
      else {
        readAddresses[position]++; // Prepare to read the next instruction
        flowControlOut[position] = false;
      }
    }
    else {
      addr = static_cast<Data>(in[position].read()).getData();
      w = data.read(addr);
    }

    AddressedWord aw(w, connections[position]);
    out[position].write(aw);

    if(DEBUG) cout<<"Read "<<data.read(addr)<<" from address "<<addr<<endl;
  }
  // Don't allow any more requests if we are unable to send the results.
  else flowControlOut[position].write(false);

}

/* Carry out a write for the transaction at input "position". */
void MemoryMat::write(Word w, int position) {
  int addr = storeAddresses[position];
  data.write(w, addr);

  storeAddresses[position] = -1;
  flowControlOut[position].write(true);   // Is this necessary?

  if(DEBUG) cout << "Wrote " << w << " to address " << addr << endl;
}

/* Update the current connections to this memory. */
void MemoryMat::updateControl() {

  ChannelRequest req = static_cast<ChannelRequest>(in[CONTROL_INPUT].read());

  int port = req.getPort();
  int type = req.getType();

  if(type == ChannelRequest::SETUP) {
    // Set up the connection if there isn't one there already
    if(connections[port] == NO_CONNECTION) {
      connections[port] = req.getReturnChannel();
      flowControlOut[port].write(true);

      // could make port 0 the control port, so it has an output to send replies on
      // send ACK (from where? out[port]?)

      if(DEBUG) cout << "Set up connection at memory port " << port << endl;
    }
    else {
      // send NACK (from where?)
    }
  }
  else {  // Tear down the channel
    connections[port] = NO_CONNECTION;
    flowControlOut[port].write(false);

    if(DEBUG) cout << "Tore down connection at port " << port << endl;
  }

}

/* Initialise the contents of this memory to the Words in the given vector. */
void MemoryMat::storeData(std::vector<Word>& data) {
  for(unsigned int i=0; i<data.size(); i++) {
    this->data.write(data[i], i);
  }
}

MemoryMat::MemoryMat(sc_module_name name, int ID) :
    TileComponent(name, ID),
    data(MEMORY_SIZE),
    connections(NUM_CLUSTER_INPUTS-1, NO_CONNECTION),
    storeAddresses(NUM_CLUSTER_INPUTS-1, -1),
    readAddresses(NUM_CLUSTER_INPUTS-1, -1),
    readingIPK(NUM_CLUSTER_INPUTS-1, false) {

  // Register methods
  SC_METHOD(doOp);
  sensitive << clock.pos();
  dont_initialize();

  end_module(); // Needed because we're using a different constructor

}

MemoryMat::~MemoryMat() {

}
