/*
 * MemoryOperation.h
 *
 *  Base class for all memory operations.
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORY_OPERATIONS_MEMORYOPERATION_H_
#define SRC_TILE_MEMORY_OPERATIONS_MEMORYOPERATION_H_

#include "../../Memory/MemoryTypes.h"
#include "../Flit.h"
#include "../Identifier.h"
#include "../../Network/NetworkTypes.h"

class MemoryBase;

// Different types of data which may be accessed by memory operations.
enum MemoryData {
  MEMORY_BYTE,
  MEMORY_HALFWORD,
  MEMORY_WORD,
  MEMORY_INSTRUCTION,
  MEMORY_CACHE_LINE,
  MEMORY_METADATA
};

// Options for aligning requests to different boundaries. All requested
// memory addresses are rounded down to the nearest multiple of the given data
// type.
// TODO: not sure this is needed. Might just need to give memories a MemoryData
// and they return a number of bytes.
enum MemoryAlignment {
  ALIGN_BYTE,
  ALIGN_HALFWORD,
  ALIGN_WORD,
  ALIGN_CACHE_LINE
};

class MemoryOperation {

protected:

  MemoryOperation(MemoryAddr address,       // Address to access
                  MemoryMetadata metadata,  // Cache/scratchpad, skip, etc.
                  ChannelID returnAddress,  // Network destination for results
                  MemoryData datatype,      // Type of data to be accessed
                  MemoryAlignment alignment,// Alignment of initial address
                  uint iterations,          // Consecutive data to access
                  bool reads,               // Does this operation read memory?
                  bool writes);             // Does this operation write memory?

public:

  virtual ~MemoryOperation();

  // Assign this operation to the given memory, and say whether the memory is
  // acting as an L1, L2, etc.
  virtual void assignToMemory(MemoryBase& memory, MemoryLevel level);

  // Returns whether the operation is destined for this memory.
  virtual bool needsForwarding() const;

  // Send a flit back to the return address, without going through the whole
  // `execute` process.
  virtual void forwardResult(unsigned int data, bool isInstruction = false);

  // Perform one cycle's worth of preparations for an operation. This method is
  // non-blocking, so should be executed repeatedly until preconditionsMet()
  // returns true.
  virtual void prepare();

  // Returns whether the operation is ready to begin.
  virtual bool preconditionsMet() const = 0;

  // Perform one cycle's worth of execution of the operation. This method is
  // non-blocking, so should be executed repeatedly until complete() returns
  // true.
  virtual void execute();

  // Returns whether the operation has completed.
  virtual bool complete() const;

  // Returns whether any further payloads are expected.
  virtual bool awaitingPayload() const;

  // Returns the number of payload flits yet to arrive.
  virtual uint payloadFlitsRemaining() const = 0;

  // Returns whether any further results are to be sent.
  virtual bool resultsToSend() const;

  // Returns the number of result flits yet to be sent.
  virtual uint resultFlitsRemaining() const = 0;

  // Ensure that the cache line this operation accesses is in the cache.
  void allocateLine() const;

  // Ensure that there is a space available for the cache line this operation
  // accesses.
  void validateLine() const;

  // Invalidate the cache line this operation accesses.
  void invalidateLine() const;

  // Flush the cache line this operation accesses.
  void flushLine() const;

  // Return whether the data to be accessed is currently in the memory.
  bool inCache() const;

  // Return the memory address accessed by this operation.
  MemoryAddr getAddress() const;

  // Return the position in SRAM accessed by this operation.
  SRAMAddress getSRAMAddress() const;

  // Return the metadata supplied with the original request.
  MemoryMetadata getMetadata() const;

  // Return which level of memory hierarchy this operation intends to act in.
  MemoryLevel getMemoryLevel() const;

  // Return the network address to which all results are sent.
  ChannelID getDestination() const;

  // Returns whether the memory should be treated as a cache or scratchpad.
  MemoryAccessMode getAccessMode() const;

  // Record that this operation missed in the cache.
  void notifyCacheMiss();

  // Access whether this operation missed in the cache.
  bool wasCacheMiss() const;

  // A textual representation of the operation.
  string toString() const;

  // Convert the request header into a form that can be sent on the network.
  virtual NetworkRequest toFlit() const;

protected:

  // Perform one iteration of an operation. Return whether the iteration was
  // completed. If the operation did not complete (e.g. waiting for data to
  // arrive over the network), the iteration will be started from the
  // beginning, so no permanent changes should be made unless the operation
  // completes successfully.
  virtual bool oneIteration();

  // Read data from memory. The location is taken from the local sramAddress
  // variable.
  uint32_t readMemory();

  // Write data to memory. The location is taken from the local sramAddress
  // variable.
  void writeMemory(uint32_t data);

  // Returns whether the memory has a payload flit ready for this operation.
  bool payloadAvailable() const;

  // Retrieves a payload flit from the memory.
  unsigned int getPayload();

  // Send a word to the chosen destination.
  void sendResult(unsigned int data, bool isInstruction = false);

private:

  // Perform safety checks before writing any data.
  void preWriteCheck() const;

  // Determine whether `assignToMemory` has been called yet. This must happen
  // before any processing can occur.
  bool memoryAssigned() const;

  // Round `address` down to something valid, given the alignment. Typically
  // this will be the largest multiple of the data size which is less than the
  // address.
  MemoryAddr align(MemoryAddr address, MemoryAlignment alignment) const;

  // Return the size in bytes of the given data type.
  size_t getSize(MemoryData datatype) const;

public:

  const uint            id;             // Unique identifier for this request

protected:

  // Information about the operation to be performed.
  const MemoryAddr      address;        // Position in address space
  const MemoryMetadata  metadata;       // Various information modifying the operation
  const ChannelID       returnAddress;  // Channel to send results to

  // Information about the location where the operation is performed.
  MemoryBase*           memory;         // The memory processing this operation
  MemoryLevel           level;          // Whether this is the L1, L2, etc.
  SRAMAddress           sramAddress;    // Position in SRAM of base address

  // Dynamic information about the progress of the operation.
  uint                  totalIterations;// Number of accesses to perform
  uint                  iterationsComplete;

private:

  const MemoryData      datatype;       // Type of data being accessed
  size_t                dataSize;       // Size of each accessed value in bytes
  const MemoryAlignment alignment;      // Round address down to multiple of this
  const bool            readsMemory;    // Does this operation read memory?
  const bool            writesMemory;   // Does this operation modify memory?

  SRAMAddress           addressOffset;  // Amount to add to address/sramAddress
  bool                  cacheMiss;      // Did this operation miss in the cache?
  bool                  endOfPacketSeen;// Have we seen an EOP instruction?

  static uint operationCount;           // Counter used to generate unique IDs
};

#endif /* SRC_TILE_MEMORY_OPERATIONS_MEMORYOPERATION_H_ */
