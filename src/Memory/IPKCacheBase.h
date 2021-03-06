/*
 * IPKCacheBase.h
 *
 *  Created on: 11 Jul 2012
 *      Author: db434
 */

#ifndef IPKCACHEBASE_H_
#define IPKCACHEBASE_H_

#include "../Datatype/Instruction.h"
#include "../Types.h"
#include "../Utility/ISA.h"
#include "../Utility/LoopCounter.h"
#include "MemoryTypes.h"

using sc_core::sc_event;

class IPKCacheBase {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  IPKCacheBase(const std::string& name, size_t size, size_t numTags,
               size_t maxIPKLength);
  virtual ~IPKCacheBase();

//============================================================================//
// Methods
//============================================================================//

public:

  // Returns whether the given address matches any of the tags.
  virtual CacheIndex lookup(const MemoryAddr tag);

  // Returns the next item in the cache.
  virtual const Instruction read();

  // Writes new data to the cache. The position to write to, and the tag for
  // the data, are already known. Returns the position written to.
  virtual CacheIndex write(const Instruction inst);

  // Set the tag of the most recently written instruction to the given value.
  virtual void setTag(MemoryAddr tag);

  // Jump to a new instruction at a given offset.
  virtual void jump(const JumpOffset offset);

  // Returns the remaining number of entries in the cache.
  virtual size_t remainingSpace() const;

  // Returns whether the cache is empty. Note that even if a cache is empty,
  // it is still possible to access its contents if an appropriate tag is
  // looked up.
  virtual bool empty() const;

  // Returns whether the cache is full.
  virtual bool full() const;

  // Event triggered whenever an instruction is read for the first time. This
  // is the signal that a credit should be sent (if necessary).
  const sc_event& dataConsumedEvent() const;

  // Return whether this core is allowed to send out a new fetch request.
  // It is not allowed to send the request if there is not room for a maximum-
  // size packet.
  virtual bool canFetch() const;

  // Abort execution of this instruction packet, and resume when the next packet
  // arrives.
  virtual void cancelPacket();

  // Store some initial instructions in the cache. Returns the cache location
  // to which the first instruction was written.
  virtual CacheIndex storeCode(const std::vector<Instruction>& code);

  size_t size() const;

  // Low-level access methods - use with caution!
  CacheIndex getReadPointer() const;
  CacheIndex getWritePointer() const;
  void setReadPointer(CacheIndex pos);
  void setWritePointer(CacheIndex pos);

  const Instruction& debugRead(uint pos) const;

protected:

  // Returns the position of the instruction from the given memory location, or
  // NOT_IN_CACHE if it is not available.
  virtual CacheIndex cacheIndex(const MemoryAddr address) const = 0;

  virtual void setTag(const CacheIndex position, const MemoryAddr tag) = 0;

  // Determine which position to read from next. Executed immediately before
  // performing the read.
  virtual void updateReadPointer() = 0;

  // Determine which position to write to next. Executed immediately before
  // performing the write.
  virtual void updateWritePointer() = 0;

  // Returns the position that data with the given address tag should be stored.
//  virtual CacheIndex getPosition(const MemoryAddr& address) = 0;

  virtual void incrementWritePos();
  virtual void incrementReadPos();

  virtual size_t getFillCount() const;

private:

  // Instrumentation helper methods.
  void tagActivity();
  void dataActivity();

//============================================================================//
// Local state
//============================================================================//

protected:

  static const CacheIndex NOT_IN_CACHE = -1;
  static const MemoryAddr DEFAULT_TAG  = 0xFFFFFFFF;

  const std::string name;

  // The maximum number of instructions in a single packet. Allows us to be
  // sure that any packet will fit in the cache when requested.
  const size_t maxIPKLength;

  // All accesses to tags must be through getTag and setTag, as different cache
  // configurations use tags differently.
  std::vector<MemoryAddr>  tags;
  std::vector<Instruction> data;
  std::vector<bool>        fresh;

  // Current instruction pointer and refill pointer.
  LoopCounter readPointer, writePointer;

  JumpOffset jumpAmount;
  bool finishedPacketRead, finishedPacketWrite;

  // When the read and write pointers are in the same place, we use this to
  // tell whether the cache is full or empty.
  bool lastOpWasARead;

private:

  sc_event dataConsumed;

  // Keep track of which cycles the tag and data arrays were active so we can
  // estimate potential savings of clock gating.
  cycle_count_t lastTagActivity, lastDataActivity;

};

#endif /* IPKCACHEBASE_H_ */
