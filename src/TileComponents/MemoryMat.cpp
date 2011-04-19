/*
 * MemoryMat.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "MemoryMat.h"
#include "ChannelMapEntry.h"
#include "ConnectionStatus.h"
#include "../Datatype/AddressedWord.h"
#include "../Datatype/Instruction.h"
#include "../Datatype/MemoryRequest.h"
#include "../Utility/Instrumentation.h"

/* Every clock cycle, check for new inputs, and carry out any pending
 * operations. */
void MemoryMat::mainLoop() {
  while(true) {
    wait(clock.posedge_event());
    creditssent = 0;

    // This is the start of a new cycle: nothing useful has happened yet. If an
    // operation takes place, this will be set to true.
    active = false;

    checkInputs();
    performOperations();
    updateIdle();
  }
}

/* Check all inputs for new data, and put it into the corresponding input
 * buffers. If there is no operation at a particular port, prepare the newly-
 * received request for execution. */
void MemoryMat::checkInputs() {
  for(PortIndex i=0; i<MEMORY_INPUT_PORTS; i++) {
    if(validDataIn[i].read()) {
      AddressedWord input = dataIn[i].read();

      // Store the first input channel ID so it doesn't have to be recomputed?
      ChannelIndex channel = input.channelID() - inputChannelID(id, 0);

      if(input.portClaim()) {
        if(DEBUG) cout << "Channel (" << id << "," << (int)channel << ") was claimed by "
                       << outputPortString(input.payload().toInt()) << endl;

        // assert(inputBuffers_[channel].empty()); ?
        // Claim this input channel for the sending output channel. Store
        // the output channel's address so we can send credits back.
        creditDestinations_[channel] = input.payload().toInt();
        sendCredit(channel);
      }
      else {
        if(DEBUG) cout << this->name() << " channel " << (int)channel
                       << " received " << input.payload() << endl;
        // Flow control means there should be at least one buffer space available.
        assert(!inputBuffers_[channel].full());
        inputBuffers_[channel].write(input.payload());
      }

      // Acknowledge the received data.
      ackDataIn[i].write(true);
      wait(sc_core::SC_ZERO_TIME);
      ackDataIn[i].write(false);
    }
  }
}

/* Determine which of the pending operations are allowed to occur (if any),
 * and execute them. */
void MemoryMat::performOperations() {
  // Don't do anything if there is nothing to do, or if we can't send the result.
  if((activeConnections == 0 && inputBuffers_.empty()) || outputBuffer_.full())
    return;

  // Choose which buffer the next operation should come from.
  ChannelIndex request = nextRequest();

  if(request < MEMORY_INPUT_CHANNELS) {
    active = true;
    ConnectionStatus& connection = connections_[request];

    if(connection.idle())         newOperation(request);
    else if(connection.isRead())  read(request);
    else if(connection.isWrite()) write(request);
  }
}

/* Carry out a read for the transaction at the given input. */
void MemoryMat::read(ChannelIndex input) {
  // If we are reading, we should be allowed to send the result.
  assert(!validDataOut[0].read());

  ConnectionStatus& connection = connections_[input];
  MemoryAddr addr = connection.address();
  ChannelID returnAddr = sendTable_[input].destination();
  Word data;

  if(DEBUG) cout << "Reading from memory " << id << ", address " << addr << ": ";

  // Collect some information about the read.
  bool endOfPacket = true;
  bool isInstruction = false;

  if(connection.readingIPK()) {
    isInstruction = true;
    data = readWord(addr);

    // May need to read additional data to get a whole instruction.
    if(BYTES_PER_INSTRUCTION > BYTES_PER_WORD) {
      uint32_t first  = (uint32_t)data.toInt();
      uint32_t second = (uint32_t)readWord(addr+BYTES_PER_WORD).toInt();
      uint64_t total  = ((uint64_t)first << 32) + second;
      data = Word(total);
    }

    if(static_cast<Instruction>(data).endOfPacket()) {
      // If this is the end of the packet, end this operation.
      connection.clear(); activeConnections--;
    }
    else {
      endOfPacket = false;
      connection.incrementAddress(); // Prepare to read the next instruction.
    }
  }
  else {
    if(connection.byteAccess())          data = readByte(addr);
    else if(connection.halfWordAccess()) data = readHalfWord(addr);
    else                                 data = readWord(addr);

    connection.clear(); activeConnections--;
  }

  if(DEBUG) cout << data << endl;
  Instrumentation::l1Read(addr, isInstruction);

  // Send the result of the read.
  AddressedWord aw(data, returnAddr);
  if(!endOfPacket) aw.notEndOfPacket();
  queueData(aw, input);
}

/* Carry out a write for the transaction at the given input port. */
void MemoryMat::write(ChannelIndex port) {
  // There must be some data in the input buffer to write.
  assert(!inputBuffers_[port].empty());

  ConnectionStatus& connection = connections_[port];
  MemoryAddr addr = connection.address();
  Word data = inputBuffers_[port].read();
  sendCredit(port); // Removed something from the buffer so send a credit.

  Instrumentation::l1Write(addr);
  if(DEBUG) cout << "Writing " << data << " to memory " << id
                 << ", address " << addr << endl;

  if(connection.byteAccess())          writeByte(addr, data);
  else if(connection.halfWordAccess()) writeHalfWord(addr, data);
  else                                 writeWord(addr, data);

  // If we're expecting more writes, update the address, otherwise clear it.
  if(connection.streaming()) connection.incrementAddress();
  else {
    connection.clear(); activeConnections--;
  }
}

/* Returns the channel of the next request which should be accepted. */
ChannelIndex MemoryMat::nextRequest() {
  // Move on from where we last stopped.
  ++currChannel;

  for(unsigned int i=0; i<inputBuffers_.size(); i++, ++currChannel) {
    ChannelIndex channel = currChannel.value();
    if((!inputBuffers_[channel].empty() && connections_[channel].idle()) || canAcceptRequest(channel)) {
      return channel;
    }
  }

  // If we have got this far, there were no suitable requests.
  return 0xFF;
}

/* Tells whether we are able to carry out a waiting operation at the given
 * port. */
bool MemoryMat::canAcceptRequest(ChannelIndex i) const {
  // See if there is an operation waiting or in progress.
  bool operation = !connections_[i].idle();
  if(!operation) return false;

  // See if this is a read, requiring a result to be sent.
  bool willSend = connections_[i].isRead();

  // If a result will need to be sent, check that the flow control signal
  // allows this.
  bool canSend = !validDataOut[0].read() && sendTable_[i].canSend();

  // Writes require data to write, so see if it has arrived yet.
  bool moreData = !inputBuffers_[i].empty();

  return (willSend && canSend) || (!willSend && moreData);
}

/* There is no active operation at the given port, but there is one waiting in
 * the input buffer. Prepare the operation, and carry it out if appropriate. */
void MemoryMat::newOperation(ChannelIndex port) {
  assert(!inputBuffers_[port].empty());

  MemoryRequest request = static_cast<MemoryRequest>(inputBuffers_[port].read());
  sendCredit(port); // Removed something from buffer, so send credit.

  if(request.isSetup()) {
    if(DEBUG) cout << this->name() << " set-up connection at port "
                   << (int)port << endl;
    updateControl(port, request.address());
  }
  else if(request.isReadRequest()) {
    connections_[port].readAddress(request.address(), request.operation());
    if(request.streaming()) connections_[port].startStreaming();
    activeConnections++;

    // Assuming it's possible to set up a read and perform a read in one cycle.
    if(!validDataOut[0].read()) read(port);
  }
  else if(request.isWriteRequest()) {
    connections_[port].writeAddress(request.address(), request.operation());
    if(request.streaming()) connections_[port].startStreaming();
    activeConnections++;
  }
  else cerr << "Warning: Unknown memory request." << endl;
}

/* Set up a new connection at the given input port. All requests to this port
 * are to send their results back to returnAddr. */
void MemoryMat::updateControl(ChannelIndex input, ChannelID returnAddr) {
  active = true;

  // We don't have to do any safety checks because we assume that all
  // connections are statically scheduled.
  connections_[input].clear();

  // Wait until all data sent has been consumed before changing the destination.
  while(!sendTable_[input].haveAllCredits()) {
    wait(creditsIn[0].default_event());
  }
  sendTable_[input].destination(returnAddr);

  // Set up a connection to the port we are now sending to: send the output
  // port's ID, and flag it as a setup message.
  int portID = outputChannelID(id, input);
  AddressedWord aw(Word(portID), returnAddr, true);

  // Send the port claim onto the network.
  queueData(aw, input);
}

/* Update this memory's idle status, saying whether anything happened or was
 * waiting to happen this cycle. */
void MemoryMat::updateIdle() {
  if(active == !idle.read()) return;
  idle.write(!active);
  Instrumentation::idle(id, !active);
}

/* Send a credit, showing that an item was removed from the corresponding input
 * buffer. Would ideally like to combine the credit sending with the buffer
 * reading so we can be sure that the credit is always sent. */
void MemoryMat::sendCredit(ChannelIndex position) {
  ChannelID returnAddr = creditDestinations_[position];

  // TODO: only use port 0
  unsigned int output = creditssent++;
  assert(output < MEMORY_INPUT_PORTS);

  // TODO: create a separate thread to do this part so useful work can still
  // be done.
  creditsOut[output].write(AddressedWord(1, returnAddr));
  validCreditOut[output].write(true);

  if(DEBUG) cout << "Sent credit from (" << id << "," << (int)position << ") to "
                 << TileComponent::outputPortString(returnAddr) << endl;

  wait(ackCreditOut[output].posedge_event());
  validCreditOut[output].write(false);
}

void MemoryMat::queueData(AddressedWord& data, MapIndex channel) {
  // Block until this channel has enough credits to send.
  while(!sendTable_[channel].canSend()) {
    wait(creditsIn[0].default_event());
    // Wait a fraction longer to ensure credit count is updated first.
//    wait(sc_core::SC_ZERO_TIME);
  }

  if(DEBUG) cout << "Sending " << data.payload()
                 << " from (" << id << "," << (int)channel << ") to "
                 << TileComponent::inputPortString(data.channelID())
                 << endl;

  assert(!outputBuffer_.full());
  outputBuffer_.write(data);
  sendTable_[channel].removeCredit();
}

void MemoryMat::sendData() {
  while(true) {
    wait(clock.posedge_event());

    if(!outputBuffer_.empty()) {
      dataOut[0].write(outputBuffer_.read());
      validDataOut[0].write(true);

      wait(ackDataOut[0].posedge_event());
      validDataOut[0].write(false);
    }
  }
}

void MemoryMat::newCredit() {
  // Note: this may not work if we have multiple tiles: it relies on all of the
  // addresses being neatly aligned.
  ChannelIndex targetCounter = creditsIn[0].read().channelID() % MEMORY_OUTPUT_CHANNELS;

  if(DEBUG) cout << "Received credit at (" << id << "," << (int)targetCounter
                 << ")" << endl;

  sendTable_[targetCounter].addCredit();
}

/* Initialise the contents of this memory to the Words in the given vector. */
void MemoryMat::storeData(const vector<Word>& data, MemoryAddr location) {
  assert(data.size() + location <= data_.size());

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

void MemoryMat::print(MemoryAddr start, MemoryAddr end) const {
  cout << "\nContents of " << this->name() << ":" << endl;
  data_.print(start/BYTES_PER_WORD, end/BYTES_PER_WORD);
}

const Word MemoryMat::readWord(MemoryAddr addr) const {
  assert(addr%BYTES_PER_WORD == 0);
  return data_.read(addr/BYTES_PER_WORD);
}

const Word MemoryMat::readHalfWord(MemoryAddr addr) const {
  assert(addr%(BYTES_PER_WORD/2) == 0);
  return data_.read(addr/BYTES_PER_WORD).getHalfWord((addr%BYTES_PER_WORD)/(BYTES_PER_WORD/2));
}

const Word MemoryMat::readByte(MemoryAddr addr) const {
  return data_.read(addr/BYTES_PER_WORD).getByte(addr%BYTES_PER_WORD);
}

void MemoryMat::writeWord(MemoryAddr addr, Word data) {
  assert(addr%BYTES_PER_WORD == 0);
  writeMemory(addr, data);
}

void MemoryMat::writeHalfWord(MemoryAddr addr, Word data) {
  assert(addr%(BYTES_PER_WORD/2) == 0);
  Word current = readWord(addr - (addr%BYTES_PER_WORD));
  Word updated = current.setHalfWord((addr%BYTES_PER_WORD)/(BYTES_PER_WORD/2), data.toInt());
  writeMemory(addr, updated);
}

void MemoryMat::writeByte(MemoryAddr addr, Word data) {
  Word current = readWord(addr - (addr%BYTES_PER_WORD));
  Word updated = current.setByte(addr%BYTES_PER_WORD, data.toInt());
  writeMemory(addr, updated);
}

void MemoryMat::writeMemory(MemoryAddr addr, Word data) {
  data_.write(data, addr/BYTES_PER_WORD);
}

MemoryMat::MemoryMat(sc_module_name name, ComponentID ID) :
    TileComponent(name, ID, MEMORY_INPUT_PORTS, MEMORY_OUTPUT_PORTS),
    data_(MEMORY_SIZE, string(name)),
    creditDestinations_(MEMORY_INPUT_CHANNELS),
    connections_(MEMORY_INPUT_CHANNELS),
    sendTable_(MEMORY_OUTPUT_CHANNELS),
    inputBuffers_(MEMORY_INPUT_CHANNELS, CHANNEL_END_BUFFER_SIZE, string(name)),
    outputBuffer_(CHANNEL_END_BUFFER_SIZE, string(name)),
    currChannel(MEMORY_INPUT_CHANNELS) {

  wordsLoaded_ = 0;
  currChannel = MEMORY_INPUT_CHANNELS - 1;

  SC_THREAD(mainLoop);

  SC_METHOD(newCredit);
  sensitive << creditsIn[0];
  dont_initialize();

  SC_THREAD(sendData);

  Instrumentation::idle(id, true);

}
