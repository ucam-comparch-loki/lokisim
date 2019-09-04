/*
 * InstructionPacketCache.h
 *
 * This class acts as a wrapper for the inner behavioural model. The wrapper
 * adds SystemC ports and events and various other functionality.
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#ifndef INSTRUCTIONPACKETCACHE_H_
#define INSTRUCTIONPACKETCACHE_H_

#include "../../../LokiComponent.h"
#include "../../../Memory/IPKCacheBase.h"
#include "../../../Network/Interface.h"
#include "../../../Network/NetworkTypes.h"
#include "InstructionStore.h"

class FetchStage;
class Word;

class InstructionPacketCache : public LokiComponent,
                               public InstructionStore,
                               public network_sink_ifc<Word> {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput clock;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(InstructionPacketCache);
  InstructionPacketCache(sc_module_name name, const cache_parameters_t& params);
  virtual ~InstructionPacketCache();

//============================================================================//
// Methods
//============================================================================//

public:

  // Initialise the contents of the cache with a list of instructions.
  void storeCode(const std::vector<Instruction>& instructions);

  // Read the next instruction from the cache.
  virtual const Instruction read();

  // Write a new instruction to the cache.
  virtual void write(const Instruction inst);

  // Return the position within the cache that the instruction with the given
  // tag is, or NOT_IN_CACHE if it is not there.
  virtual CacheIndex lookup(MemoryAddr tag);

  // Return the memory address of the most recently read instruction. Used for
  // debug purposes only.
  virtual MemoryAddr memoryAddress() const;

  // Prepare to read a packet with the first instruction at the given location
  // within the cache.
  virtual void startNewPacket(CacheIndex position);

  // Switch to the next instruction packet immediately, rather than waiting
  // for an ".eop" marker.
  virtual void cancelPacket();

  // Tells whether the cache considers itself empty. This may be because there
  // are no instructions in the cache, or because all instructions have been
  // executed.
  virtual bool isEmpty() const;
  virtual bool isFull() const;

  // Tells whether there is enough room in the cache to fetch another
  // instruction packet. Has to assume that the packet will be of maximum
  // size.
  bool roomToFetch() const;

  // Jump to a new instruction specified by the offset.
  virtual void jump(JumpOffset offset);

  // A handle to an event which is triggered whenever something is added to or
  // removed from the cache.
  virtual const sc_event& readEvent() const;
  virtual const sc_event& writeEvent() const;


  // Network interface: some duplication with above methods.
  virtual void write(const Flit<Word>& data);
  virtual bool canWrite() const;
  virtual const sc_event& canWriteEvent() const;
  virtual const sc_event& dataConsumedEvent() const;
  virtual const Flit<Word> lastDataWritten() const;

private:

  // Tells whether an instruction was sent this cycle -- sometimes there may
  // be potential instructions both in the cache and arriving on the network
  // and we don't want to send both in the same cycle.
  bool sentInstThisCycle() const;

  FetchStage& parent() const;

//============================================================================//
// Local state
//============================================================================//

private:

  // The behavioural model of the cache.
  IPKCacheBase* cache;

  // Memory addresses of each instruction in the cache. For debug purposes only.
  vector<MemoryAddr> addresses;

  // Information used to keep track of the memory addresses of instructions.
  MemoryAddr lastReadAddr, lastWriteAddr;

  // Flag telling whether the cache missed this cycle.
  cycle_count_t timeLastChecked;
  sc_event cacheMissEvent;

  // Event which is triggered whenever an instruction is added to or removed
  // from the cache.
  sc_event cacheRead, cacheWrite;

  // Flag used to detect when we are beginning to receive a new instruction packet.
  bool finishedPacketWrite;

};

#endif /* INSTRUCTIONPACKETCACHE_H_ */
