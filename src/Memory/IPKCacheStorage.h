/*
 * IPKCacheStorage.h
 *
 * A special version of the MappedStorage class with behaviour tailored to
 * the Instruction Packet Cache.
 *
 *  Created on: 26 Jan 2010
 *      Author: db434
 */

#ifndef IPKCACHESTORAGE_H_
#define IPKCACHESTORAGE_H_

#include "MappedStorage.h"
#include "../Typedefs.h"
#include "../Utility/LoopCounter.h"
#include "../Utility/InstructionMap.h"

class Instruction;

class IPKCacheStorage : public MappedStorage<MemoryAddr, Instruction> {

//==============================//
// Constructors and destructors
//==============================//

public:

  IPKCacheStorage(const uint16_t size, const std::string& name);

//==============================//
// Methods
//==============================//

public:

  // Returns whether the given address matches any of the tags.
  // There are many different ways of fetching instructions, so provide the
  // operation too.
  virtual bool checkTags(const MemoryAddr& key, operation_t operation);

  // Returns the next item in the cache.
  virtual const Instruction& read();

  // Writes new data to the cache. The position to write to, and the tag for
  // the data, are already known.
  void write(const Instruction& inst);

  // Jump to a new instruction at a given offset.
  void jump(const JumpOffset offset);

  // Return the memory address of the currently-executing packet.
  const MemoryAddr packetAddress() const;

  // Returns the remaining number of entries in the cache.
  const uint16_t remainingSpace() const;

  // Return the memory index of the instruction sent most recently.
  const MemoryAddr getInstLocation() const;

  // Returns whether the cache is empty. Note that even if a cache is empty,
  // it is still possible to access its contents if an appropriate tag is
  // looked up.
  bool empty() const;

  // Returns whether the cache is full.
  bool full() const;

  // Return whether this core is allowed to send out a new fetch request.
  // It is not allowed to send the request if there is not room for a maximum-
  // size packet, or if any fetches are already taking place.
  bool canFetch() const;

  // Begin reading the packet which is queued up to execute next.
  void switchToPendingPacket();

  // Store some initial instructions in the cache.
  void storeCode(const std::vector<Instruction>& code);

private:

  // Returns the data corresponding to the given address.
  // Private because we don't want to use this version for IPK caches.
  virtual const Instruction& read(const MemoryAddr& key);

  // Returns the position that data with the given address tag should be stored
  virtual uint16_t getPosition(const MemoryAddr& key);

  // Returns whether the instruction stored at the given position in the cache
  // is the first instruction of its packet.
  bool startOfPacket(uint16_t cacheIndex) const;

  void incrementWritePos();
  void incrementReadPos();

  void updateFillCount();

//==============================//
// Local state
//==============================//

private:

  // A collection of information about an instruction packet and how it should
  // be executed.
  typedef struct {
    MemoryAddr memAddr;     // Memory address of this packet (mainly for debug)
    uint16_t   cacheIndex;  // Position in cache of first instruction
    bool       persistent;  // Persistent packets repeat until NXIPK is received
    bool       isFill;      // FILLs don't execute instructions when they arrive
    bool       inCache;     // Can't send a FETCH until previous one finishes

    void reset() {
      memAddr = DEFAULT_TAG; cacheIndex = NOT_IN_CACHE; inCache = false;
    }
    bool arriving() const {
      return (memAddr != DEFAULT_TAG) && !inCache && (cacheIndex != NOT_IN_CACHE);
    }
  } PacketInfo;

  // Current instruction pointer and refill pointer.
  LoopCounter readPointer, writePointer;

  uint16_t fillCount;
  uint16_t currInstBackup;   // In case it goes NOT_IN_USE and then a jump is used

  // Location of the next packet to be executed.
  // Do we want a single pending packet, or a queue of them?
  PacketInfo pendingPacket;

  // The index of the first instruction of the current instruction packet.
  PacketInfo currentPacket;

  // For debug purposes, we store the memory address which each instruction is
  // from. This allows us to set breakpoints easily, and also allows us to
  // determine which parts of the program are the hotspots.
  vector<MemoryAddr> locations;

  // Store the most recent instruction address, so it can be accessed easily.
  MemoryAddr previousLocation;

  static const uint16_t   NOT_IN_CACHE = -1;
  static const MemoryAddr DEFAULT_TAG  = 0xFFFFFFFF;

};

#endif /* IPKCACHESTORAGE_H_ */
