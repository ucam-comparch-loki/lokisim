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

  void storeCode(std::vector<Instruction>& instructions);
  bool isEmpty();

private:

  void insertInstruction();
  void lookup();
  void finishedRead();
  void updateRTF();
  void write();

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
  sc_signal<bool> writeNotify1, writeNotify2, writeNotify3;

};

#endif /* INSTRUCTIONPACKETCACHE_H_ */
