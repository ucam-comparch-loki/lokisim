/*
 * IPKCacheBase.h
 *
 *  Created on: 11 Jul 2012
 *      Author: db434
 */

#ifndef IPKCACHEBASE_H_
#define IPKCACHEBASE_H_

#include "../Datatype/Instruction.h"
#include "../Typedefs.h"
#include "../Utility/LoopCounter.h"
#include "../Utility/InstructionMap.h"


typedef uint CacheIndex;
typedef uint TagIndex;

class IPKCacheBase {

//==============================//
// Constructors and destructors
//==============================//

public:

  IPKCacheBase(const size_t size, const size_t numTags, const std::string& name);

//==============================//
// Methods
//==============================//

public:

  // Returns whether the given address matches any of the tags.
  // There are many different ways of fetching instructions, so provide the
  // operation too.
  virtual bool checkTags(const MemoryAddr address, const opcode_t operation);

  // Returns the next item in the cache.
  virtual const Instruction read();

  // Writes new data to the cache. The position to write to, and the tag for
  // the data, are already known.
  virtual void write(const Instruction inst);

  // Jump to a new instruction at a given offset.
  virtual void jump(const JumpOffset offset);

  // Return the memory address of the currently-executing packet.
  virtual const MemoryAddr packetAddress() const;

  // Returns the remaining number of entries in the cache.
  virtual const size_t remainingSpace() const;

  // Return the memory index of the instruction sent most recently.
  virtual const MemoryAddr getInstLocation() const;

  // Returns whether the cache is empty. Note that even if a cache is empty,
  // it is still possible to access its contents if an appropriate tag is
  // looked up.
  virtual bool empty() const;

  // Returns whether the cache is full.
  virtual bool full() const;

  // Returns whether this cache is stalled, waiting for instructions to arrive
  // from memory.
  virtual bool stalled() const;

  // Return whether this core is allowed to send out a new fetch request.
  // It is not allowed to send the request if there is not room for a maximum-
  // size packet, or if any fetches are already taking place.
  virtual bool canFetch() const;

  virtual bool packetInProgress() const;

  // Begin reading the packet which is queued up to execute next.
  virtual void switchToPendingPacket();

  // Abort execution of this instruction packet, and resume when the next packet
  // arrives.
  virtual void cancelPacket();

  // Store some initial instructions in the cache.
  virtual void storeCode(const std::vector<Instruction>& code);

  size_t size() const;

protected:

  // Returns the position of the instruction from the given memory location, or
  // NOT_IN_CACHE if it is not available.
  virtual CacheIndex cacheIndex(const MemoryAddr address) const = 0;

  virtual MemoryAddr getTag(const CacheIndex position) const = 0;
  virtual void setTag(const CacheIndex position, const MemoryAddr tag) = 0;

  // Determine which position to read from next. Executed immediately before
  // performing the read.
  virtual void updateReadPointer() = 0;

  // Determine which position to write to next. Executed immediately before
  // performing the write.
  virtual void updateWritePointer() = 0;

  // Returns the position that data with the given address tag should be stored.
//  virtual CacheIndex getPosition(const MemoryAddr& address) = 0;

  // Returns whether the instruction stored at the given position in the cache
  // is the first instruction of its packet.
  virtual bool startOfPacket(CacheIndex position) const;

  virtual void incrementWritePos();
  virtual void incrementReadPos();

  virtual void updateFillCount();

private:

  // Instrumentation helper methods.
  void tagActivity();
  void dataActivity();

//==============================//
// Local state
//==============================//

protected:

  const std::string name;

  // All accesses to tags must be through getTag and setTag, as different cache
  // configurations use tags differently.
  std::vector<MemoryAddr>  tags;
  std::vector<Instruction> data;

  // For debug purposes, we store the memory address which each instruction is
  // from. This allows us to set breakpoints easily, and also allows us to
  // determine which parts of the program are the hotspots.
  std::vector<MemoryAddr>  locations;

  static const CacheIndex NOT_IN_CACHE = -1;
  static const MemoryAddr DEFAULT_TAG  = 0xFFFFFFFF;

  // A collection of information about an instruction packet and how it should
  // be executed.
  typedef struct {
    MemoryAddr memAddr;     // Memory address of this packet (mainly for debug)
    CacheIndex cacheIndex;  // Position in cache of first instruction
    bool       persistent;  // Persistent packets repeat until NXIPK is received
    bool       execute;     // Should these instructions be executed immediately?
    bool       inCache;     // Can't send a FETCH until previous one finishes

    void reset() {
      memAddr = DEFAULT_TAG; cacheIndex = NOT_IN_CACHE; inCache = false; execute = true;
    }
    bool arriving() const {
      return (memAddr != DEFAULT_TAG) && !inCache && (cacheIndex != NOT_IN_CACHE);
    }
  } PacketInfo;

  // Current instruction pointer and refill pointer.
  LoopCounter readPointer, writePointer;

  JumpOffset jumpAmount;
  bool finishedPacketRead, finishedPacketWrite;

  // When the read and write pointers are in the same place, we use this to
  // tell whether the cache is full or empty.
  bool lastOpWasARead;

  size_t fillCount;

  // Location of the next packet to be executed.
  // Do we want a single pending packet, or a queue of them?
  PacketInfo pendingPacket;

  // The index of the first instruction of the current instruction packet.
  PacketInfo currentPacket;

private:

  // Keep track of which cycles the tag and data arrays were active so we can
  // estimate potential savings of clock gating.
  cycle_count_t lastTagActivity, lastDataActivity;

};

#endif /* IPKCACHEBASE_H_ */
