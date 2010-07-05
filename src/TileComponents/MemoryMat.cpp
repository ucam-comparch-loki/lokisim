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
#include "../Utility/Instrumentation.h"

const int MemoryMat::CONTROL_INPUT = NUM_CLUSTER_INPUTS - 1;

/* Look through all inputs for new data. Determine whether this data is the
 * start of a new transaction or the continuation of an existing one. Then
 * carry out the first/next step of the transaction. */
void MemoryMat::doOp() {

  // Inputs are always allowed to the control port
  flowControlOut[CONTROL_INPUT].write(1);

  if(in[CONTROL_INPUT].event()) updateControl();

  for(int i=0; i<NUM_CLUSTER_INPUTS-1; i++) {

    // Attempt to send anything waiting in buffers. Update the flow control
    // before (not after), to avoid multiple operations arriving in the same
    // cycle (flow control gets updated the cycle after removing from buffers).
    flowControlOut[i].write(buffers[i].remainingSpace());
    if(!buffers[i].isEmpty() && flowControlIn[i].read()) {
      out[i].write(buffers.read(i));
      continue;
    }

    ConnectionStatus& connection = connections[i];

    // Don't allow an input if there is no connection set up
    if(!connection.isActive()) {
      flowControlOut[i].write(0);
      continue;
    }

    if(in[i].event()) {
      // If there isn't an existing transaction, this must be a read
      if(connection.isIdle()) {
        if(DEBUG) cout << "Received new memory request at memory " << id <<
                          ", input " << i << ": ";

        MemoryRequest r = static_cast<MemoryRequest>(in[i].read());
        if(r.isReadRequest()) {
          if(DEBUG) cout << "read from address " << r.getAddress() << endl;
          if(r.isIPKRequest()) {
            connection.startStreaming();
            connection.setReadAddress(r.getAddress());
          }
          read(i);
        }
        // Dealing with a write request
        else {
          if(DEBUG) cout << "store to address " << r.getAddress() << endl;
          connection.setWriteAddress(r.getAddress());
          if(r.getOperation() == MemoryRequest::STADDR) {
            connection.startStreaming();
          }
        }
      }
      // If there is an existing transaction, this must be data to be written
      else write(in[i].read(), i);
    }
    // If there is a read transaction in progress, send the next value
    else if(connection.readingIPK()) {
      read(i);
    }
    // No new input and no existing transaction => accept new transactions
    else {
      flowControlOut[i].write(buffers[i].remainingSpace());
    }

  }

  updateIdle();

}

/* Carry out a read for the transaction at input "position". */
void MemoryMat::read(int position) {

  ConnectionStatus& connection = connections[position];
  Word w;
  int addr;
  int channel = connection.getChannel();

  if(connection.readingIPK()) {
    addr = connection.getAddress();
    w = data.read(addr);

    if(static_cast<Instruction>(w).endOfPacket()) {
      connection.clear();
      flowControlOut[position].write(buffers[position].remainingSpace());
    }
    else {
      connection.incrementAddress(); // Prepare to read the next instruction

      // Pretend the buffer is full to allow the read sequence to complete.
      flowControlOut[position].write(0);
    }
  }
  else {
    addr = static_cast<Data>(in[position].read()).getData();
    w = data.read(addr);
    connection.clear();
  }

  AddressedWord aw(w, channel);

  Instrumentation::memoryRead();

  if(DEBUG) cout << "Read " << data.read(addr) << " from memory " << id <<
                    ", address " << addr << endl;

  if(flowControlIn[position].read()) {
    // Flow control allowing, send the value we just read.
    out[position].write(aw);
  }
  else {
    // Buffer this request until it is able to complete.
    buffers.write(aw, position);

    // Update the amount of space remaining in the buffer.
    flowControlOut[position].write(buffers[position].remainingSpace());
  }

}

/* Carry out a write for the transaction at input "position". */
void MemoryMat::write(Word w, int position) {
  ConnectionStatus& connection = connections[position];

  int addr = connection.getAddress();
  data.write(w, addr);

  // If we're expecting more writes, update the address, otherwise clear it.
  if(connection.isStreaming()) connection.incrementAddress();
  else                         connection.clear();

  flowControlOut[position].write(buffers[position].remainingSpace());

  Instrumentation::memoryWrite();

  if(DEBUG) cout << "Wrote " << w << " to memory " << id <<
                    ", address " << addr << endl;
}

/* Update the current connections to this memory. */
void MemoryMat::updateControl() {

  ChannelRequest req = static_cast<ChannelRequest>(in[CONTROL_INPUT].read());

  int port = req.getPort();
  int type = req.getType();

  ConnectionStatus& connection = connections[port];

  if(type == ChannelRequest::SETUP) {
    // Set up the connection if there isn't one there already
    if(!connection.isActive()) {
      connection.setChannel(req.getReturnChannel());
      flowControlOut[port].write(buffers[port].remainingSpace());

      // could make port 0 the control port, so it has an output to send replies on
      // send ACK (from where? out[port]?)

      if(DEBUG) cout << "Set up connection at memory " << id <<
                        ", port " << port << endl;
    }
    else {
      if(DEBUG) cout << "Unable to set up connection at memory " << id <<
                        ", port " << port << endl;
      // send NACK (from where?)
    }
  }
  else {  // Tear down the channel
    connection.teardown();

    // Pretend the buffer is full to stop new data arriving
    flowControlOut[port].write(0);

    if(DEBUG) cout << "Tore down connection at memory " << id <<
                      ", port " << port << endl;
  }

}

void MemoryMat::updateIdle() {
  bool isIdle = true;

  for(unsigned int i=0; i<connections.size(); i++) {
    ConnectionStatus& c = connections[i];

    // Note: we can't rely on the isActive method, since the user may forget
    // to take down the connection.
    if(c.isActive() && c.isStreaming()) isIdle = false;
  }

  idle.write(isIdle);

  Instrumentation::idle(id, isIdle,
      sc_core::sc_time_stamp().to_default_time_units());
}

// Estimate of area in um^2 obtained from Cacti.
double MemoryMat::area() const {
  return 0.0; // Non-linear: see spreadsheet
}

double MemoryMat::energy() const {
  return 0.0;
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
    connections(NUM_CLUSTER_INPUTS-1),
    buffers(NUM_CLUSTER_INPUTS-1, 1) {   // Buffer size = 1 => can try expanding

  // Register methods
  SC_METHOD(doOp);
  sensitive << clock.pos();
  dont_initialize();

  end_module(); // Needed because we're using a different Component constructor

}

MemoryMat::~MemoryMat() {

}
