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
#include "../../../Datatype/Address.h"
#include "../../../Datatype/Instruction.h"

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

  SC_HAS_PROCESS(InstructionPacketCache);
  InstructionPacketCache(sc_module_name name);
  virtual ~InstructionPacketCache();

//==============================//
// Methods
//==============================//

public:

  // Initialise the contents of the cache with a list of instructions.
  void storeCode(std::vector<Instruction>& instructions);

  Instruction read();

  void write(Instruction inst);

  // Return the index into the current packet of the current instruction.
  int  getInstIndex() const;

  // Tells whether the cache considers itself empty. This may be because there
  // are no instructions in the cache, or because all instructions have been
  // executed.
  bool isEmpty() const;

  // See if an instruction packet is in the cache, using its address as a tag,
  // and if so, prepare to execute it. Returns whether or not the packet is
  // in the cache.
  bool lookup(Address addr);

  // Jump to a new instruction specified by the offset.
  void jump(int8_t offset);

private:

  // Update the output flow control signal.
  void updateFlowControl();

  // Update the output holding the address of the currently-executing
  // instruction packet.
//  void updatePacketAddress(Address addr);

  // Update whether or not we are in persistent execution mode, where we
  // execute a single instruction packet repeatedly.
//  void updatePersistent();

  // Send the chosen instruction, instToSend. There are multiple writers so
  // a separate method is needed.
//  void write();

  // Perform any necessary tasks when the end of an instruction packet has been
  // reached.
  void endOfPacketTasks();

  // Perform any necessary tasks when starting to read a new instruction packet.
  void startOfPacketTasks();

  // Tells whether an instruction was sent this cycle -- sometimes there may
  // be potential instructions both in the cache and arriving on the network
  // and we don't want to send both in the same cycle.
  bool sentInstThisCycle() const;

//==============================//
// Local state
//==============================//

private:

  IPKCacheStorage cache;
  Buffer<Address> addresses;

  // The cycle during which the most recent instruction was sent -- allows us
  // to avoid sending more than one in a single cycle.
  int             lastInstSent;

  // The instruction sent has been read, so we can now prepare the next one.
  bool            outputWasRead;

  // We have just finished writing an instruction packet.
  bool            startOfPacket;

  // We have just finished reading an instruction packet.
  bool            finishedPacketRead;

  // The address of the pending packet. We store it in case it needs to be
  // refetched because it has been overwritten.
  Address         pendingPacket;

};

#endif /* INSTRUCTIONPACKETCACHE_H_ */
