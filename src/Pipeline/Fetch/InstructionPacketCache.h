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

#include "../../Component.h"
#include "../../Memory/IPKCacheStorage.h"
#include "../../Memory/Buffer.h"
#include "../../Datatype/Address.h"
#include "../../Datatype/Instruction.h"

class InstructionPacketCache : public Component {

public:
/* Ports */
  sc_in<Address> address;
  sc_in<Instruction> in;
  sc_in<bool> clock, readInstruction;
  sc_out<Instruction> out;
  sc_out<bool> cacheHit, isRoomToFetch;

/* Constructors and destructors */
  SC_HAS_PROCESS(InstructionPacketCache);
  InstructionPacketCache(sc_core::sc_module_name name, int ID);
  virtual ~InstructionPacketCache();

private:
/* Methods */
  void insertInstruction();
  void lookup();
  void finishedRead();
  void updateRTF();
  void write();

/* Local state */
  IPKCacheStorage<Address, Instruction> cache;
  Buffer<Address> addresses;
  Instruction instToSend;
  bool sentNewInst, outputWasRead, startOfPacket;

/* Signals (wires) */
  sc_signal<bool> writeNotify1, writeNotify2; // Signal that there is data to send

};

#endif /* INSTRUCTIONPACKETCACHE_H_ */
