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

#include "../../../Component.h"
#include "../../../Memory/IPKCacheBase.h"
#include "../../../Network/NetworkTypedefs.h"

class FetchStage;
class Word;

class InstructionPacketCache : public Component {

//==============================//
// Ports
//==============================//

public:

  ClockInput  clock;

  sc_in<Word> instructionIn;

  // Signal telling the flow control unit whether there is space left in the cache.
  ReadyOutput flowControl;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(InstructionPacketCache);
  InstructionPacketCache(sc_module_name name, const ComponentID& ID);
  virtual ~InstructionPacketCache();

//==============================//
// Methods
//==============================//

public:

  // Initialise the contents of the cache with a list of instructions.
  void storeCode(const std::vector<Instruction>& instructions);

  // Read the next instruction from the cache.
  Instruction read();

  // Write a new instruction to the cache.
  void write(const Instruction inst);

  // Return the index into the current packet of the current instruction.
  MemoryAddr getInstAddress() const;

  // Tells whether the cache considers itself empty. This may be because there
  // are no instructions in the cache, or because all instructions have been
  // executed.
  bool isEmpty() const;

  // Tells whether there is enough room in the cache to fetch another
  // instruction packet. Has to assume that the packet will be of maximum
  // size.
  bool roomToFetch() const;

  // See if an instruction packet is in the cache, using its address as a tag,
  // and if so, prepare to execute it. Returns whether or not the packet is
  // in the cache.
  // There are many different ways of fetching instructions, so provide the
  // operation too.
  bool lookup(const MemoryAddr addr, opcode_t operation);

  // Jump to a new instruction specified by the offset.
  void jump(const JumpOffset offset);

  // Switch to the next instruction packet immediately, rather than waiting
  // for an ".eop" marker.
  void nextIPK();

  // Tells whether the cache is in the middle of issuing an instruction packet
  // to the pipeline.
  bool packetInProgress() const;

  // A handle to an event which is triggered whenever something is added to or
  // removed from the cache.
  const sc_event& fillChangedEvent() const;

private:

  void receivedInst();

  // Update the output flow control signal.
  void sendCredit();

  // Tells whether an instruction was sent this cycle -- sometimes there may
  // be potential instructions both in the cache and arriving on the network
  // and we don't want to send both in the same cycle.
  bool sentInstThisCycle() const;

  FetchStage* parent() const;

//==============================//
// Local state
//==============================//

private:

  // The behavioural model of the cache.
  IPKCacheBase* cache;

  // Flag telling whether the cache missed this cycle.
  cycle_count_t timeLastChecked;
  sc_event cacheMissEvent;

  // Event which is triggered whenever an instruction is added to or removed
  // from the cache.
  sc_event cacheFillChanged;

};

#endif /* INSTRUCTIONPACKETCACHE_H_ */
