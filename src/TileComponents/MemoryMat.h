/*
 * MemoryMat.h
 *
 * A memory bank which is accessed over the network. There will usually be
 * multiple MemoryMats in each Tile.
 *
 * Protocol for communicating with a memory:
 *  1. Claim a port in the usual way (invisible to the programmer/compiler).
 *     This is currently done automatically whenever setchmap is executed.
 *  2. Send a MemoryRequest to that port, specifying the channel ID to send
 *     results to, and saying that the MemoryRequest is to set up a connection:
 *     MemoryRequest(channel id, MemoryRequest::SETUP);
 *  3. Use the load and store instructions to send addresses/data to the
 *     memory.
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#ifndef MEMORYMAT_H_
#define MEMORYMAT_H_

#include "TileComponent.h"
#include "../Memory/AddressedStorage.h"
#include "../Memory/BufferArray.h"

using std::vector;

class ChannelMapEntry;
class ConnectionStatus;
class Word;

class MemoryMat: public TileComponent {

//==============================//
// Ports
//==============================//

// Inherited from TileComponent:
//   clock
//   in
//   out
//   flowControlIn
//   flowControlOut
//   idle

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(MemoryMat);
  MemoryMat(sc_module_name name, ComponentID ID);

//==============================//
// Methods
//==============================//

public:

  // Initialise the contents of this memory to the Words in the given vector.
  virtual void storeData(const vector<Word>& data, MemoryAddr location=0);

  // Print the contents of this memory.
  virtual void print(MemoryAddr start=0, MemoryAddr end=MEMORY_SIZE) const;

  // Return the value at the given address.
  virtual const Word readWord(MemoryAddr addr) const;
          const Word readHalfWord(MemoryAddr addr) const;
  virtual const Word readByte(MemoryAddr addr) const;
  virtual void writeWord(MemoryAddr addr, Word data);
          void writeHalfWord(MemoryAddr addr, Word data);
  virtual void writeByte(MemoryAddr addr, Word data);

private:

  // Every clock cycle, check for inputs, and carry out any pending operations.
  void mainLoop();

  // Check all input ports for new data, and put it into the buffers.
  void checkInputs();

  // Determine which of the pending operations are allowed to occur (if any),
  // and execute them.
  void performOperations();

  // Carry out a read for the transaction at input "position".
  void read(ChannelIndex position);

  // Carry out a write for the transaction at input "position".
  void write(ChannelIndex position);

  // Returns a vector of all input ports at which there are memory operations
  // ready to execute.
  vector<ChannelIndex>& allRequests();

  // Determine which output port a request at a particular input should use.
  ChannelIndex outputToUse(ChannelIndex input) const;

  // Tells whether we are able to carry out a waiting operation at the given
  // port.
  bool canAcceptRequest(ChannelIndex port) const;

  // Update the contents of the data array. A "write" without any status
  // checking.
  void writeMemory(MemoryAddr addr, Word data);

  // Update the connection at the given port so that results of memory reads
  // are sent back to returnAddr.
  void updateControl(ChannelIndex port, ChannelID returnAddr);

  // Choose which of the provided inputs are allowed to perform an operation
  // concurrently. Leaves only the allowed inputs in the vector.
  void arbitrate(vector<ChannelIndex>& inputs);

  // Update the output signal telling whether the memory is idle.
  void updateIdle();

  // Prepare the next available operation at the given input port.
  void newOperation(ChannelIndex position);

  // Send a flow control credit from a particular port.
  void sendCredit(ChannelIndex position);

  void sendData(AddressedWord& data, MapIndex channel);

  // Slight optimisation: signal that data has arrived, so we only look through
  // the inputs when we know there is something to find.
  void newData();

  // Perform any necessary actions when a new credit arrives.
  void newCredit();

//==============================//
// Local state
//==============================//

private:

  // The data stored by this memory.
  AddressedStorage<Word> data_;

  // The addresses to send credits back to.
  vector<ChannelID> creditDestinations_;

  // Information on the channels set up with each of this memory's inputs.
  vector<ConnectionStatus> connections_;

  // An equivalent of the channel map table in a core. Stores addresses and
  // credit counts of all output channels.
  vector<ChannelMapEntry> sendTable_;

  // A queue of operations from each input port.
  BufferArray<Word> inputBuffers_;

  // The number of words we have initially put in this memory. Allows multiple
  // files to be put into the same memory.
  int wordsLoaded_;

  // Tells whether this memory has done something productive this cycle.
  bool active;

  // Flag telling whether data has arrived since we last checked.
  bool newData_;

  // Used for arbitration. Stores the input port for which a request was most
  // recently granted.
  int lastAccepted;

  // Optimisation: only see which work can be done if there is actually work
  // to do.
  int activeConnections;

};

#endif /* MEMORYMAT_H_ */
