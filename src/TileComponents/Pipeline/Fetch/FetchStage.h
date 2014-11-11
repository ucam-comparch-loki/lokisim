/*
 * FetchStage.h
 *
 * Fetch stage of the pipeline. Responsible for receiving instructions from
 * the network and feeding the decode stage a (near) constant supply of
 * instructions in the correct order.
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#ifndef FETCHSTAGE_H_
#define FETCHSTAGE_H_

#include "../PipelineStage.h"
#include "InstructionPacketCache.h"
#include "InstructionPacketFIFO.h"
#include "../../../Memory/BufferStorage.h"
#include "../../../Network/NetworkTypedefs.h"
#include "../../../Utility/Blocking.h"

class FetchStage : public PipelineStage, public Blocking {

private:

  // A collection of information about an instruction packet and how it should
  // be executed.
  typedef struct {
    MemoryAddr memAddr;     // Memory address of this packet (mainly for debug)
    InstLocation location;  // Location of first instruction of the packet
    bool       persistent;  // Persistent packets repeat until NXIPK is received
    bool       execute;     // Should these instructions be executed immediately?
    bool       inCache;     // Is the packet completely in the cache?

    void reset() {
      memAddr = DEFAULT_TAG;
      location.index = NOT_IN_CACHE;
      inCache = false;
      execute = true;
//      location.component = UNKNOWN;
      persistent = false;
    }
    bool arriving() const {
      return active() && !inCache && (location.index != NOT_IN_CACHE);
    }
    bool active() const {
      return memAddr != DEFAULT_TAG;
    }
  } PacketInfo;

  // Information about each instruction packet we want to bring into the cache.
  typedef struct {
    MemoryAddr address;     // Memory address of the desired packet.
    opcode_t   operation;   // Fetch operation used to request the packet.
    ChannelID  destination; // Network address through which the packet can be reached.
    ChannelIndex returnAddress; // Return instructions to {0 = FIFO, 1 = cache}.
  } FetchInfo;

//==============================//
// Ports
//==============================//

public:

// Inherited from PipelineStage:
//   sc_in<bool>         clock

  // The input instruction to be sent to the instruction packet cache.
  sc_in<Word>         iToCache;

  // The input instruction to be sent to the instruction packet FIFO.
  sc_in<Word>         iToFIFO;

  // A flow control signal from each of the two instruction inputs.
  LokiVector<ReadyOutput> oFlowControl;
  LokiVector<ReadyOutput> oDataConsumed;

  DataOutput          oFetchRequest;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(FetchStage);
  FetchStage(sc_module_name name, const ComponentID& ID);

//==============================//
// Methods
//==============================//

public:

  // Store some instructions in the Instruction Packet Cache.
  void          storeCode(const std::vector<Instruction>& instructions);

  // Return the memory address of the last instruction sent.
  MemoryAddr    getInstAddress() const;

  // Tell whether it is possible to check cache tags at this time. It may not
  // be possible if there is already the maximum number of packets queued up.
  bool          canCheckTags() const;

  // Queue up an address to be looked up in the cache. Provide some extra
  // information to allow the fetch request to be sent, if necessary.
  void          checkTags(MemoryAddr addr,              // Address of packet
                          opcode_t operation,           // Type of fetch request
                          ChannelID channel,            // Memory channel
                          ChannelIndex returnChannel);  // Core's instruction input

  // Jump to a different instruction in the Instruction Packet Cache.
  void          jump(const JumpOffset offset);

protected:

  virtual void  reportStalls(ostream& os);

private:

  virtual void  execute();

  // Continuously read instructions and pass them to the next pipeline stage.
  void          readLoop();

  // Continuously check whether new instructions are needed, fetch them, and
  // put them into the instruction stores.
  void          writeLoop();

  // Tells whether the packet from location a is currently in the cache.
  // There are many different ways of fetching instructions, so provide the
  // operation too.
  bool          inCache(const MemoryAddr a, opcode_t operation);

  // Tells whether there is room in the cache to fetch another instruction
  // packet, assuming the packet is of maximum size.
  bool          roomToFetch() const;

  // Recompute whether this pipeline stage is stalled.
  virtual void  updateReady();

  // Move to the next instruction packet on demand, rather than when reaching
  // a ".eop" marker.
  void          nextIPK();

  // Signal to this pipeline stage that a new packet has started to arrive, and
  // tell where it is. Returns the memory address (tag) of the packet.
  MemoryAddr    newPacketArriving(const InstLocation& location);

  // Signal that a complete instruction packet has now arrived over the network.
  // This may allow a new fetch command to be sent.
  void          packetFinishedArriving();

  void          reachedEndOfPacket();

  // Returns whether all instruction stores are empty.
  bool          waitingForInstructions();

  // Find where in the FIFO or cache the given tag is. If it isn't anywhere,
  // returns InstLocation(UNKNOWN, NOT_IN_CACHE).
  const InstLocation lookup(const MemoryAddr addr);

  InstructionStore& currentInstructionSource();

  // Override PipelineStage's implementation.
  virtual void  getNextInstruction();

//==============================//
// Components
//==============================//

private:

  InstructionPacketCache    cache;
  InstructionPacketFIFO     fifo;

  friend class InstructionPacketCache;
  friend class InstructionPacketFIFO;

//==============================//
// Local state
//==============================//

private:

  enum ReadState {
    RS_READY,       // Have no instruction packets to read
    RS_READ         // Read instructions and pass them to the pipeline
  };

  enum WriteState {
    WS_READY,       // Have no instructions to fetch
    WS_CHECK_TAGS,  // Check whether instructions need to be fetched
  };

  ReadState  readState;
  WriteState writeState;

  // Information on the packet currently being executed, and optionally, the
  // packet which is due to execute next.
  // Do we want a single pending packet, or a queue of them?
  PacketInfo currentPacket, pendingPacket;

  // Various flags which may mean we aren't executing instructions sequentially.
  bool jumpedThisCycle, finishedPacketRead;

  // If this pipeline stage is stalled, we assume the whole pipeline is stalled.
  bool stalled;

  // Keep track of the current packet address so we can execute programs through
  // the FIFO (where there are no tags). In practice, this would be handled in
  // the compiler, but we don't have this functionality yet.
  MemoryAddr previousFetch;


  // Buffer to store fetches as we wait for any outstanding operations to
  // complete.
  BufferStorage<FetchInfo> fetchBuffer;

  // Event which is triggered whenever a packet finishes arriving.
  sc_event packetArrivedEvent;

  // Event which is triggered whenever the pending packet changes. This means
  // that there is a new tag look-up to be done.
  sc_event newPacketAvailable;

};

#endif /* FETCHSTAGE_H_ */
