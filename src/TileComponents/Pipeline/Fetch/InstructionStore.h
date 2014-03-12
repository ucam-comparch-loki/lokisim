/*
 * InstructionStore.h
 *
 * Interface shared by the instruction packet cache and instruction packet FIFO.
 *
 *  Created on: 31 Aug 2012
 *      Author: db434
 */

#ifndef INSTRUCTIONSTORE_H_
#define INSTRUCTIONSTORE_H_

#include "../../../Typedefs.h"

class Instruction;

static const CacheIndex NOT_IN_CACHE = -1;
static const MemoryAddr DEFAULT_TAG  = 0xFFFFFFFF;

enum InstructionSource {IPKFIFO, IPKCACHE, UNKNOWN};

// All information required to find an instruction packet.
typedef struct {
  InstructionSource component;
  CacheIndex        index;
} InstLocation;

class InstructionStore {

public:

  // Return the position within the store that the instruction with the given
  // tag is, or NOT_IN_CACHE if it is not there.
  virtual CacheIndex lookup(MemoryAddr tag) = 0;

  // Returns whether there is still an instruction packet at the given position.
  // In rare circumstances, the packet may be overwritten between first checking
  // for it and beginning to execute it.
  virtual bool packetExists(CacheIndex position) const = 0;

  // Write an instruction to the next available space.
  virtual void write(const Instruction inst) = 0;

  // Prepare to read a packet with the first instruction at the given location
  // within this store.
  virtual void startNewPacket(CacheIndex position) = 0;

  // Perform any necessary tidying when we abort an instruction packet.
  virtual void cancelPacket() = 0;

  // Return the instruction immediately after the previously read instruction
  // (or otherwise if there has been a jump or a new packet).
  virtual const Instruction read() = 0;

  // Immediately jump to a relative position in the instruction store.
  virtual void jump(JumpOffset amount) = 0;

  // Returns whether there are any instructions left to read. Note that it is
  // possible to get instructions from an "empty" store by jumping or looking
  // up the appropriate tag.
  virtual bool isEmpty() const = 0;
  virtual bool isFull() const = 0;

  // Returns an event which is triggered whenever an instruction is read or
  // written to the store.
  virtual const sc_event& fillChangedEvent() const = 0;

  virtual ~InstructionStore() {};

};

#endif /* INSTRUCTIONSTORE_H_ */
