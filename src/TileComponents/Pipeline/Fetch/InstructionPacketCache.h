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

  // Clock.
  sc_in<bool>         clock;

  // The instruction received from the network.
  sc_in<Instruction>  instructionIn;

  // A signal which changes to signify that an instruction should be read
  // from the cache (rather than the instruction FIFO).
  sc_in<bool>         readInstruction;

  // The instruction read from the cache.
  sc_out<Instruction> instructionOut;

  // The offset to jump to in the cache.
  sc_in<short>        jumpOffset;

  // The address tag being looked up in the cache.
  sc_in<Address>      address;

  // Signals that we should keep executing the same packet.
  sc_in<bool>         persistent;

  // The address of the currently-executing instruction packet.
  sc_out<Address>     currentPacket;

  // Signals that the looked-up address matches an instruction packet in the
  // cache.
  sc_out<bool>        cacheHit;

  // Signals that there is room to accommodate another maximum-sized
  // instruction packet in the cache.
  sc_out<bool>        isRoomToFetch;

  // A signal which changes to show that the next packet to executed needs
  // to be fetched again.
  sc_out<bool>        refetch;

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

  // Return the index into the current packet of the current instruction.
  int  getInstIndex() const;

  // Tells whether the cache considers itself empty. This may be because there
  // are no instructions in the cache, or because all instructions have been
  // executed.
  bool isEmpty() const;

private:

  //Put a received instruction into the cache.
  void insertInstruction();

  // See if an instruction packet is in the cache, using its address as a tag,
  // and if so, prepare to execute it.
  void lookup();

  // Jump to a new instruction specified by the offset received by jumpOffset.
  void jump();

  // An instruction was read from the cache, so prepare the next instruction,
  // and change to another instruction packet if necessary.
  void finishedRead();

  // Update the signal saying whether there is enough room to fetch another
  // packet.
  void updateRTF();

  // Update the output holding the address of the currently-executing
  // instruction packet.
  void updatePacketAddress(Address addr);

  // Update whether or not we are in persistent execution mode, where we
  // execute a single instruction packet repeatedly.
  void updatePersistent();

  // Send the chosen instruction, instToSend. There are multiple writers so
  // a separate method is needed.
  void write();

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
  Instruction     instToSend;

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

  // Signal that there is an instruction ready to send.
  sc_event readyToWrite;

  // Signal that we are now reading the first instruction of an instruction
  // packet.
  sc_event startingPacket;

};

#endif /* INSTRUCTIONPACKETCACHE_H_ */
