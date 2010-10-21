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
  checkInputs();

  for(uint i=0; i<NUM_CLUSTER_INPUTS-1; i++) {
    ConnectionStatus& connection = connections_[i];

    if(!inputBuffers_[i].isEmpty() && !connection.readingIPK()) {

      // If there is no connection set up, this must be a request to make a
      // connection.
      if(!connection.active()) {
        MemoryRequest req = static_cast<MemoryRequest>(inputBuffers_[i].read());

        if(req.isSetup()) {
          if(DEBUG) cout << this->name() << " set-up connection at port " << i << endl;
          updateControl(i, req.address());
          sendCredit(i);
        }
        else {
          // If there is no connection set up and we receive something which
          // isn't a set up message, something is wrong.
          cerr << "Error: unexpected input at " << this->name() << ": "
               << req << endl;
          // throw exception?
        }
      }
      // If there isn't an existing transaction, this must be a read
      else if(connection.idle()) {
        if(DEBUG) cout << "Received new memory request at memory " << id <<
                          ", input " << i << ": ";

        // If we are not allowed to send any data (because of flow control),
        // we will not be able to carry out any reads, so only peek at the
        // next command until we know that we are able to complete it.
        MemoryRequest req = static_cast<MemoryRequest>(inputBuffers_[i].peek());

        if(req.isSetup()) {
          if(DEBUG) cout << "set-up connection at port " << i << endl;
          updateControl(i, req.address());
          inputBuffers_[i].read();
          sendCredit(i);
        }
        else if(req.isReadRequest()) {
          if(DEBUG) cout << "read from address " << req.address() << endl;
          if(flowControlIn[i]) {
            if(req.isIPKRequest()) {
              connection.startStreaming();
              connection.readAddress(req.address());
            }
            read(i);
            inputBuffers_[i].read();  // Remove the transaction from the queue
            sendCredit(i);
          }
        }
        // Dealing with a write request
        else {
          if(DEBUG) cout << "store to address " << req.address() << endl;
          connection.writeAddress(req.address());
          if(req.operation() == MemoryRequest::STADDR) {
            connection.startStreaming();
          }
          inputBuffers_[i].read();  // Remove the transaction from the queue
          sendCredit(i);
        }
      } // end new operation
      // If there is an existing transaction, this must be data to be written
      else {
        write(inputBuffers_[i].read(), i);
        sendCredit(i);
      }
    } // end not busy and work to do
    // If there is a read transaction in progress, send the next value
    else if(flowControlIn[i] && connection.readingIPK()) {
      read(i);
      // Don't send a credit because no input was consumed.
    }
  }

  updateIdle();
}

/* Carry out a read for the transaction at input "position". We must be able
 * to complete the read (i.e. flowControl[position] must be true). */
void MemoryMat::read(int position) {

  ConnectionStatus& connection = connections_[position];
  Word w;
  int addr;
  int channel = connection.channel();

  if(connection.readingIPK()) {
    addr = connection.address();
    w = data_.read(addr/BYTES_PER_WORD);

    if(static_cast<Instruction>(w).endOfPacket()) {
      connection.clear();
    }
    else {
      connection.incrementAddress(); // Prepare to read the next instruction
    }
  }
  else {
    addr = static_cast<MemoryRequest>(inputBuffers_[position].peek()).address();

    if(connection.isByteAccess()) {
      // Extract an individual byte.
      uint readVal = (uint)data_.read(addr/BYTES_PER_WORD).toInt();
      int offset = addr % BYTES_PER_WORD;
      int shiftAmount = 8*offset;
      int returnVal = readVal >> shiftAmount;
      w = Word(returnVal & 255);
    }
    else {
      if(addr&3) cerr << "Misaligned address: " << addr << endl;
      w = data_.read(addr/BYTES_PER_WORD);
    }

    connection.clear();

  }

  AddressedWord aw(w, channel);

  Instrumentation::memoryRead();

  if(DEBUG) cout << "Read " << data_.read(addr/BYTES_PER_WORD) << " from memory "
                 << id << ", address " << addr << endl;

  out[position].write(aw);

}

/* Carry out a write for the transaction at input "position". */
void MemoryMat::write(Word w, int position) {
  ConnectionStatus& connection = connections_[position];

  int addr = connection.address();
  if(!connection.isByteAccess()) data_.write(w, addr/BYTES_PER_WORD);
  else {
    // If dealing with bytes, need to read the old value and only update
    // part of it.
    uint currVal = (uint)data_.read(addr/BYTES_PER_WORD).toInt();
    int offset = addr % BYTES_PER_WORD;
    int shiftAmount = offset*8;
    uint mask = ~(255 << shiftAmount);
    currVal &= mask;
    uint newVal = w.toInt() & 255;
    currVal &= (newVal << shiftAmount);
    Word newWord(currVal);
    data_.write(newWord, addr/BYTES_PER_WORD);
  }

  // If we're expecting more writes, update the address, otherwise clear it.
  if(connection.streaming()) connection.incrementAddress();
  else                       connection.clear();

  Instrumentation::memoryWrite();

  if(DEBUG) cout << "Wrote " << w << " to memory " << id <<
                    ", address " << addr << endl;
}

void MemoryMat::updateControl(uint8_t port, uint16_t returnAddr) {
  // We don't have to do any safety checks because we assume that all
  // connections are statically orchestrated.
  ConnectionStatus& connection = connections_[port];
  connection.clear();
  connection.channel(returnAddr);

  if(port >= NUM_CLUSTER_OUTPUTS) {
    cerr << "Error: memories don't yet have the same number of outputs"
         << " as inputs." << endl;
  }

  // Set up a connection to the port we are now sending to: send the output
  // port's ID, and flag it as a setup message.
  int portID = id*NUM_CLUSTER_OUTPUTS + port;
  AddressedWord aw(Word(portID), returnAddr, true);
  out[port].write(aw);
}

void MemoryMat::updateIdle() {
  bool isIdle = true;

  for(uint i=0; i<connections_.size(); i++) {
    ConnectionStatus& c = connections_[i];

    // Note: we can't rely on the isActive method, since the user may forget
    // to take down the connection.
    if(c.active() && c.streaming()) isIdle = false;

    // TODO: memory is active if it was read from in this cycle
  }

  idle.write(isIdle);

  Instrumentation::idle(id, isIdle,
      sc_core::sc_time_stamp().to_default_time_units());
}

void MemoryMat::checkInputs() {
  for(uint i=0; i<NUM_CLUSTER_INPUTS; i++) {
    if(!inputBuffers_[i].isFull() && in[i].event()) {
      inputBuffers_[i].write(in[i].read());
    }
  }
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
void MemoryMat::storeData(std::vector<Word>& data, int location) {
  // wordsLoaded is used to keep track of where to put new data. If we have
  // been given a location, we already know where to put the data, so do not
  // need wordsLoaded.
  int startIndex = (location==0) ? wordsLoaded_ : location;
  if(location==0) wordsLoaded_ += data.size();

  if(startIndex<0 || startIndex>=(int)data_.size()) {
    cerr << "Error: trying to store data to memory " << id << ", position "
         << startIndex << endl;
    throw std::exception();
  }

  for(uint i=0; i<data.size(); i++) {
    this->data_.write(data[i], startIndex + i);
  }
}

void MemoryMat::print(int start, int end) const {
  cout << "\nContents of " << this->name() << ":" << endl;
  data_.print(start/BYTES_PER_WORD, end/BYTES_PER_WORD);
}

Word MemoryMat::getMemVal(uint32_t addr) const {
  return data_.read(addr/BYTES_PER_WORD);
}

MemoryMat::MemoryMat(sc_module_name name, int ID) :
    TileComponent(name, ID),
    data_(MEMORY_SIZE),
    connections_(NUM_CLUSTER_INPUTS),
    inputBuffers_(NUM_CLUSTER_INPUTS, 4) {

  wordsLoaded_ = 0;

  // Register methods
  SC_METHOD(newCycle);
  sensitive << clock.neg(); // Just need to make sure flow control arrives first.
  dont_initialize();

  end_module(); // Needed because we're using a different Component constructor

}

MemoryMat::~MemoryMat() {

}
