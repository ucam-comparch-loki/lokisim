/*
 * InstructionPacketCache.h
 *
 * Cache of Instructions with FIFO behaviour.
 *
 * Stores a queue of outstanding fetch addresses, so when it starts receiving
 * a new instruction packet, the address of the instructions is known.
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#ifndef INSTRUCTIONPACKETCACHE_H_
#define INSTRUCTIONPACKETCACHE_H_

#include "../../../Component.h"
#include "../../../Memory/IPKCacheStorage.h"
#include "../../../Memory/Buffer.h"

class FetchStage;

class InstructionPacketCache : public Component {

//==============================//
// Ports
//==============================//

public:

  // Signal telling the flow control unit how much space is left in the cache.
  sc_out<int>         flowControl;

//==============================//
// Constructors and destructors
//==============================//

public:

  InstructionPacketCache(sc_module_name name);
  virtual ~InstructionPacketCache();

//==============================//
// Methods
//==============================//

public:

  // Initialise the contents of the cache with a list of instructions.
  void storeCode(std::vector<Instruction>& instructions);

  // Read the next instruction from the cache.
  Instruction read();

  // Write a new instruction to the cache.
  void write(Instruction inst);

  // Return the index into the current packet of the current instruction.
  Address getInstAddress() const;

  // Tells whether the cache considers itself empty. This may be because there
  // are no instructions in the cache, or because all instructions have been
  // executed.
  bool isEmpty() const;

  // Tells whether there is enough room in the cache to fetch another
  // instruction packet. Has to assume that the packet will be of maximum
  // size.
  bool roomToFetch() const;

  // See if an instruction packet is in the cache, using its address as a tag,
  // and if so, prepare to execute it. Returns whether or not the packet is
  // in the cache.
  bool lookup(Address addr);

  // Jump to a new instruction specified by the offset.
  void jump(int16_t offset);

  // Update whether or not we are in persistent execution mode, where we
  // execute a single instruction packet repeatedly.
  void updatePersistent(bool persistent);

private:

  // Update the output flow control signal.
  void sendCredit();

  // Update the output holding the address of the currently-executing
  // instruction packet.
  void updatePacketAddress(Address addr);

  // We have overwritten the packet which was due to execute next, so we
  // need to fetch it again.
  void refetch();

  // Perform any necessary tasks when starting to read a new instruction packet.
  void startOfPacketTasks();

  // Perform any necessary tasks when the end of an instruction packet has been
  // reached.
  void endOfPacketTasks();

  // Tells whether an instruction was sent this cycle -- sometimes there may
  // be potential instructions both in the cache and arriving on the network
  // and we don't want to send both in the same cycle.
  bool sentInstThisCycle() const;

  FetchStage* parent() const;

//==============================//
// Local state
//==============================//

private:

  IPKCacheStorage cache;
  Buffer<Address> addresses;

  // We have just finished writing an instruction packet.
  bool            startOfPacket;

  // We have just finished reading an instruction packet.
  bool            finishedPacketRead;

  // The address of the pending packet. We store it in case it needs to be
  // refetched because it has been overwritten.
  Address         pendingPacket;

};

#endif /* INSTRUCTIONPACKETCACHE_H_ */
