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

  // Signals that the looked-up address matches an instruction packet in the cache.
  sc_out<bool>        cacheHit;

  // Signals that there is room to accommodate another maximum-sized
  // instruction packet in the cache.
  sc_out<bool>        isRoomToFetch;

  // Signal telling whether new instructions from the network can be put in
  // the cache.
  sc_out<bool>        flowControl;

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

  // Tells whether the cache considers itself empty. This may be because there
  // are no instructions in the cache, or because all instructions have been
  // executed.
  bool isEmpty();

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

  // Send the chosen instruction, instToSend. There are multiple writers so
  // a separate method is needed.
  void write();

  // Perform any necessary tasks when the end of an instruction packet has been
  // reached.
  void endOfPacketTasks();

//==============================//
// Local state
//==============================//

private:

  IPKCacheStorage<Address, Instruction> cache;
  Buffer<Address> addresses;
  Instruction     instToSend;
  bool            sentNewInst, outputWasRead, startOfPacket;

//==============================//
// Signals (wires)
//==============================//

private:

  // Signal that there is an instruction ready to send.
  sc_event readyToWrite;

};

#endif /* INSTRUCTIONPACKETCACHE_H_ */
