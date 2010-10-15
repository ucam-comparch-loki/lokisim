/*
 * MemoryMat.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "MemoryMat.h"
#include "ConnectionStatus.h"
#include "../Datatype/AddressedWord.h"
#include "../Datatype/ChannelRequest.h"
#include "../Datatype/Data.h"
#include "../Datatype/Instruction.h"
#include "../Datatype/MemoryRequest.h"
#include "../Utility/Instrumentation.h"

const int MemoryMat::CONTROL_INPUT = NUM_CLUSTER_INPUTS - 1;

void MemoryMat::newCycle() {
  // Check for new input data
  for(uint i=0; i<NUM_CLUSTER_INPUTS; i++) {
    if(!inputBuffers[i].isFull() && in[i].event()) {
      inputBuffers[i].write(in[i].read());
    }
  }

  if(!inputBuffers[CONTROL_INPUT].isEmpty()) updateControl();

  for(uint i=0; i<NUM_CLUSTER_INPUTS-1; i++) {
    ConnectionStatus& connection = connections[i];

    if(!connection.isActive()) continue;

    if(!inputBuffers[i].isEmpty() && !connection.readingIPK()) {
      // If there isn't an existing transaction, this must be a read
      if(connection.isIdle()) {
        if(DEBUG) cout << "Received new memory request at memory " << id <<
                          ", input " << i << ": ";

        // If we are not allowed to send any data (because of flow control),
        // we will not be able to carry out any reads, so only peek at the
        // next command until we know that we are able to complete it.
        MemoryRequest r = static_cast<MemoryRequest>(inputBuffers[i].peek());
        if(r.isReadRequest()) {
          if(flowControlIn[i]) {
            if(DEBUG) cout << "read from address " << r.getAddress() << endl;
            if(r.isIPKRequest()) {
              connection.startStreaming();
              connection.readAddress(r.getAddress());
            }
            read(i);
            inputBuffers[i].read();  // Remove the transaction from the queue
            sendCredit(i);
          }
        }
        // Dealing with a write request
        else {
          if(DEBUG) cout << "store to address " << r.getAddress() << endl;
          connection.writeAddress(r.getAddress());
          if(r.getOperation() == MemoryRequest::STADDR) {
            connection.startStreaming();
          }
          inputBuffers[i].read();  // Remove the transaction from the queue
          sendCredit(i);
        }
      }
      // If there is an existing transaction, this must be data to be written
      else {
        write(inputBuffers[i].read(), i);
        sendCredit(i);
      }
    }
    // If there is a read transaction in progress, send the next value
    else if(flowControlIn[i] && connection.readingIPK()) {
      read(i);
    }
  }

  updateIdle();
}

/* Carry out a read for the transaction at input "position". */
void MemoryMat::read(int position) {

  ConnectionStatus& connection = connections[position];
  Word w;
  int addr;
  int channel = connection.channel();

  if(connection.readingIPK()) {
    addr = connection.address();
    w = data.read(addr/BYTES_PER_WORD);

    if(static_cast<Instruction>(w).endOfPacket()) {
      connection.clear();
    }
    else {
      connection.incrementAddress(); // Prepare to read the next instruction
    }
  }
  else {
    addr = static_cast<Data>(inputBuffers[position].peek()).data();

    if(connection.isByteAccess()) {
      // Extract an individual byte.
      uint readVal = (uint)data.read(addr/BYTES_PER_WORD).toInt();
      int offset = addr % BYTES_PER_WORD;
      int shiftAmount = 8*offset;
      int returnVal = readVal >> shiftAmount;
      w = Word(returnVal & 255);
    }
    else {
      if(addr&3) cerr << "Misaligned address: " << addr << endl;
      w = data.read(addr/BYTES_PER_WORD);
    }

    connection.clear();

  }

  AddressedWord aw(w, channel);

  Instrumentation::memoryRead();

  if(DEBUG) cout << "Read " << data.read(addr/BYTES_PER_WORD) << " from memory "
                 << id << ", address " << addr << endl;

  out[position].write(aw);

}

/* Carry out a write for the transaction at input "position". */
void MemoryMat::write(Word w, int position) {
  ConnectionStatus& connection = connections[position];

  int addr = connection.address();
  if(!connection.isByteAccess()) data.write(w, addr/BYTES_PER_WORD);
  else {
    // If dealing with bytes, need to read the old value and only update
    // part of it.
    uint currVal = (uint)data.read(addr/BYTES_PER_WORD).toInt();
    int offset = addr % BYTES_PER_WORD;
    int shiftAmount = offset*8;
    uint mask = ~(255 << shiftAmount);
    currVal &= mask;
    uint newVal = w.toInt() & 255;
    currVal &= (newVal << shiftAmount);
    Word newWord(currVal);
    data.write(newWord, addr/BYTES_PER_WORD);
  }

  // If we're expecting more writes, update the address, otherwise clear it.
  if(connection.isStreaming()) connection.incrementAddress();
  else                         connection.clear();

  Instrumentation::memoryWrite();

  if(DEBUG) cout << "Wrote " << w << " to memory " << id <<
                    ", address " << addr << endl;
}

/* Update the current connections to this memory. */
void MemoryMat::updateControl() {

  ChannelRequest req = static_cast<ChannelRequest>(inputBuffers[CONTROL_INPUT].read());

  int port = req.port();
  int type = req.type();

  ConnectionStatus& connection = connections[port];

  if(type == ChannelRequest::SETUP) {
    // Set up the connection if there isn't one there already
    if(!connection.isActive()) {
      connection.channel(req.returnChannel());

      // Set up a connection to the port we are sending to.
      int portID = id*NUM_CLUSTER_OUTPUTS + port;
      AddressedWord aw(Word(portID), req.returnChannel(), true);
      out[port].write(aw);

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

    if(DEBUG) cout << "Tore down connection at memory " << id <<
                      ", port " << port << endl;
  }

  sendCredit(CONTROL_INPUT);

}

void MemoryMat::updateIdle() {
  bool isIdle = true;

  for(uint i=0; i<connections.size(); i++) {
    ConnectionStatus& c = connections[i];

    // Note: we can't rely on the isActive method, since the user may forget
    // to take down the connection.
    if(c.isActive() && c.isStreaming()) isIdle = false;

    // TODO: memory is active if it was read from in this cycle
  }

  idle.write(isIdle);

  Instrumentation::idle(id, isIdle,
      sc_core::sc_time_stamp().to_default_time_units());
}

void MemoryMat::sendCredit(int position) {
  flowControlOut[position].write(1);
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
  for(uint i=0; i<data.size(); i++, wordsLoaded++) {
    this->data.write(data[i], wordsLoaded);
  }
}

void MemoryMat::print(int start, int end) const {
  cout << "\nContents of " << this->name() << ":" << endl;
  data.print(start/BYTES_PER_WORD, end/BYTES_PER_WORD);
}

Word MemoryMat::getMemVal(uint32_t addr) const {
  return data.read(addr/BYTES_PER_WORD);
}

MemoryMat::MemoryMat(sc_module_name name, int ID) :
    TileComponent(name, ID),
    data(MEMORY_SIZE),
    connections(NUM_CLUSTER_INPUTS-1),
    inputBuffers(NUM_CLUSTER_INPUTS, 4) {

  wordsLoaded = 0;

  // Register methods
  SC_METHOD(newCycle);
  sensitive << clock.neg(); // Just need to make sure flow control arrives first.
  dont_initialize();

  end_module(); // Needed because we're using a different Component constructor

}

MemoryMat::~MemoryMat() {

}
