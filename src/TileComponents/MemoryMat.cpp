/*
 * MemoryMat.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "MemoryMat.h"
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

    // This is the start of a new cycle: nothing useful has happened yet. If an
    // operation takes place, this will be set to true.
    active = false;

    checkInputs();

    // Wait until late in the cycle to send the outputs to simulate memory read
    // latency. Otherwise, they could arrive at their destinations one cycle
    // too soon.
    wait(clock.negedge_event());

    performOperations();
    updateIdle();
  }
}

/* Check all inputs for new data, and put it into the corresponding input
 * buffers. If there is no operation at a particular port, prepare the newly-
 * received request for execution. */
void MemoryMat::checkInputs() {
  // Skip the check if we know nothing arrived.
  if(!newData_) return;

  for(ChannelIndex i=0; i<inputBuffers_.size(); i++) {
    if(in[i].event()) {
      // Flow control means there should be at least one buffer space available.
      assert(!inputBuffers_[i].full());

      inputBuffers_[i].write(in[i].read());
    }
  }

  newData_ = false;
}

/* Determine which of the pending operations are allowed to occur (if any),
 * and execute them. */
void MemoryMat::performOperations() {
  if(activeConnections == 0 && inputBuffers_.empty()) return;

  // Collect a list of all input ports which have received a memory request
  // which has not yet been carried out, and then remove any conflicting
  // requests.
  std::vector<ChannelIndex>& requests = allRequests();
  if(requests.size() > 0) active = true;
  arbitrate(requests);

  // For each allowed request, carry out the required operation.
  for(uint j=0; j<requests.size(); j++) {
    ChannelIndex port = requests[j];
    ConnectionStatus& connection = connections_[port];

    if(connection.isRead())       read(port);
    else if(connection.isWrite()) write(port);
  }

  delete &requests;
}

/* Carry out a read for the transaction at the given input. */
void MemoryMat::read(ChannelIndex port) {
  // If we are reading, we should be allowed to send the result.
  assert(flowControlIn[port].read());

  ConnectionStatus& connection = connections_[port];
  MemoryAddr addr = connection.address();
  Word data = getMemVal(addr);
  ChannelID returnAddr = connection.channel();

  // Collect some information about the read.
  bool endOfPacket = true;
  bool isInstruction = false;

  if(connection.readingIPK()) {
    isInstruction = true;

    if(static_cast<Instruction>(data).endOfPacket()) {
      // If this is the end of the packet, end this operation and check for
      // another.
      connection.clear(); activeConnections--;
      if(!inputBuffers_[port].empty()) newOperation(port);
    }
    else {
      endOfPacket = false;
      connection.incrementAddress(); // Prepare to read the next instruction.
    }
  }
  else {
    if(connection.isByteAccess()) {
      data = data.getByte(addr % BYTES_PER_WORD);
    }
    else {
      if(addr%BYTES_PER_WORD != 0)
        cerr << "Warning: Misaligned address: " << addr << endl;
    }
    connection.clear(); activeConnections--;
  }

  // Send the result of the read.
  AddressedWord aw(data, returnAddr);
  if(!endOfPacket) aw.notEndOfPacket();
  out[port].write(aw);

  Instrumentation::memoryRead(addr, isInstruction);
  if(DEBUG) cout << "Read " << data << " from memory " << id << ", address "
                 << addr << endl;

}

/* Carry out a write for the transaction at the given input port. */
void MemoryMat::write(ChannelIndex port) {
  // There must be some data in the input buffer to write.
  assert(!inputBuffers_[port].empty());

  ConnectionStatus& connection = connections_[port];
  MemoryAddr addr = connection.address();
  Word data = inputBuffers_[port].read();
  sendCredit(port); // Removed something from the buffer so send a credit.

  if(connection.isByteAccess()) {
    Word current = getMemVal(addr);
    Word updated = current.setByte(addr%BYTES_PER_WORD, data.toInt());
    writeMemory(addr, updated);
  }
  else writeMemory(addr, data);

  // If we're expecting more writes, update the address, otherwise clear it
  // and check for another operation.
  if(connection.streaming()) connection.incrementAddress();
  else {
    connection.clear(); activeConnections--;
    if(!inputBuffers_[port].empty()) newOperation(port);
  }

  Instrumentation::memoryWrite(addr);
  if(DEBUG) cout << "Wrote " << data << " to memory " << id
                 << ", address " << addr << endl;
}

/* Returns a vector of all input ports at which there are memory operations
 * ready to execute. */
std::vector<ChannelIndex>& MemoryMat::allRequests() {
  std::vector<ChannelIndex>* requests = new std::vector<ChannelIndex>();
  for(ChannelIndex i=0; i<inputBuffers_.size(); i++) {
    // If there is something waiting in the buffer, and nothing currently
    // executing, prepare the operation in the buffer.
    if(!inputBuffers_[i].empty() &&
       (connections_[i].idle() || !connections_[i].active())) {
      newOperation(i);
    }

    if(canAcceptRequest(i)) requests->push_back(i);
  }

  return *requests;
}

/* Tells whether we are able to carry out a waiting operation at the given
 * port. */
bool MemoryMat::canAcceptRequest(ChannelIndex i) {
  // See if there is an operation waiting or in progress.
  bool operation = !connections_[i].idle();
  if(!operation) return false;

  // See if this is a read, requiring a result to be sent.
  bool willSend = connections_[i].isRead();

  // If a result will need to be sent, check that the flow control signal
  // allows this.
  bool canSend = flowControlIn[i].read();

  // Writes require data to write, so see if it has arrived yet.
  bool moreData = !inputBuffers_[i].empty();

  return (willSend && canSend) || (!willSend && moreData);
}

/* There is no active operation at the given port, but there is one waiting in
 * the input buffer. Prepare the operation, but do not carry it out yet. It
 * will be carried out by the performOperations() method, after arbitration. */
void MemoryMat::newOperation(ChannelIndex port) {
  assert(!inputBuffers_[port].empty());

  MemoryRequest request = static_cast<MemoryRequest>(inputBuffers_[port].read());
  sendCredit(port); // Removed something from buffer, so send credit.

  if(request.isSetup()) {
    if(DEBUG) cout << this->name() << " set-up connection at port "
                   << (int)port << endl;
    updateControl(port, request.address());
  }
  else {
    if(request.isReadRequest()) {
      connections_[port].readAddress(request.address(), request.byteAccess());
    }
    else if(request.isWriteRequest()) {
      connections_[port].writeAddress(request.address(), request.byteAccess());
    }
    else cerr << "Warning: Unknown memory request." << endl;

    if(request.streaming()) connections_[port].startStreaming();
    activeConnections++;
  }
}

/* Set up a new connection at the given input port. All requests to this port
 * are to send their results back to returnAddr. */
void MemoryMat::updateControl(ChannelIndex port, ChannelID returnAddr) {
  active = true;

  // We don't have to do any safety checks because we assume that all
  // connections are statically scheduled.
  ConnectionStatus& connection = connections_[port];
  connection.clear();
  connection.channel(returnAddr);

  if(port >= NUM_MEMORY_OUTPUTS) {
    cerr << "Error: memories don't yet have the same number of outputs"
         << " as inputs." << endl;
  }

  // Set up a connection to the port we are now sending to: send the output
  // port's ID, and flag it as a setup message.
  int portID = outputPortID(id, port);
  AddressedWord aw(Word(portID), returnAddr, true);
  out[port].write(aw);
}

/* Determine which of the operations at the given inputs may be carried out
 * concurrently. */
void MemoryMat::arbitrate(std::vector<ChannelIndex>& inputs) {
  // TODO: make all writes happen before any reads? Or vice versa.
  // TODO: extract arbitration into a separate component (possibly reusing
  //       network arbiters?)

  // If there are fewer requests than the number of operations we can carry
  // out, we can do them all.
  if(inputs.size() <= CONCURRENT_MEM_OPS) {
    if(inputs.size() > 0) lastAccepted = inputs.back();
    return;
  }

  uint numAccepted = 0;
  uint start = 0;

  // Find the position in the input vector to start from. This is the first
  // input after the one we most recently served.
  while(start < inputs.size() && inputs[start] <= lastAccepted) start++;

  uint i = start;

  // Round-robin scheduling: go from the last accepted position to the end.
  while(i<inputs.size()) {
    if(numAccepted<CONCURRENT_MEM_OPS) {
      i++;
      numAccepted++;
    }
    else {
      inputs.erase(inputs.begin()+i, inputs.end());
      break;
    }
  }

  // Then go from the start to the last accepted position.
  i = 0;
  while(i<start) {
    if(numAccepted<CONCURRENT_MEM_OPS) {
      i++;
      numAccepted++;
    }
    else {
      inputs.erase(inputs.begin()+i, inputs.begin()+start);
      break;
    }
  }

  if(inputs.size() > 0) lastAccepted = inputs.back();

  assert(inputs.size() <= CONCURRENT_MEM_OPS);
}

/* Update this memory's idle status, saying whether anything happened or was
 * waiting to happen this cycle. */
void MemoryMat::updateIdle() {
  idle.write(!active);
  Instrumentation::idle(id, !active);
}

/* Send a credit, showing that an item was removed from the corresponding input
 * buffer. Would ideally like to combine the credit sending with the buffer
 * reading so we can be sure that the credit is always sent. */
void MemoryMat::sendCredit(ChannelIndex position) {
  flowControlOut[position].write(1);
}

void MemoryMat::newData() {
  newData_ = true;
}

/* Initialise the contents of this memory to the Words in the given vector. */
void MemoryMat::storeData(std::vector<Word>& data, MemoryAddr location) {
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

Word MemoryMat::getMemVal(MemoryAddr addr) const {
  return data_.read(addr/BYTES_PER_WORD);
}

void MemoryMat::writeMemory(MemoryAddr addr, Word data) {
  data_.write(data, addr/BYTES_PER_WORD);
}

MemoryMat::MemoryMat(sc_module_name name, ComponentID ID) :
    TileComponent(name, ID),
    data_(MEMORY_SIZE, string(name)),
    connections_(NUM_MEMORY_INPUTS),
    inputBuffers_(NUM_MEMORY_INPUTS, CHANNEL_END_BUFFER_SIZE, string(name)) {

  flowControlOut = new sc_out<int>[NUM_MEMORY_INPUTS];
  in             = new sc_in<Word>[NUM_MEMORY_INPUTS];

  flowControlIn  = new sc_in<bool>[NUM_MEMORY_OUTPUTS];
  out            = new sc_out<AddressedWord>[NUM_MEMORY_OUTPUTS];

  wordsLoaded_ = 0;
  newData_ = false;

  SC_THREAD(mainLoop);

  SC_METHOD(newData);
  for(uint i=0; i<NUM_MEMORY_INPUTS; i++) sensitive << in[i];
  dont_initialize();

  end_module(); // Needed because we're using a different Component constructor

}

MemoryMat::~MemoryMat() {

}
