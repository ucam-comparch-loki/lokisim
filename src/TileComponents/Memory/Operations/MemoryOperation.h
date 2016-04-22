/*
 * MemoryOperation.h
 *
 *  Base class for all memory operations.
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#ifndef SRC_TILECOMPONENTS_MEMORY_OPERATIONS_MEMORYOPERATION_H_
#define SRC_TILECOMPONENTS_MEMORY_OPERATIONS_MEMORYOPERATION_H_

#include "../../../Datatype/Flit.h"
#include "../../../Datatype/Identifier.h"
#include "../../../Network/NetworkTypedefs.h"
#include "../MemoryTypedefs.h"

class MemoryInterface;

class MemoryOperation {

protected:

  MemoryOperation(MemoryAddr address,
                  MemoryMetadata metadata,
                  MemoryInterface& memory,
                  MemoryLevel level,
                  ChannelID destination,
                  unsigned int payloadFlits,
                  unsigned int maxResultFlits);

public:

  virtual ~MemoryOperation();

  // Returns whether the operation is destined for this memory.
  virtual bool needsForwarding() const;

  // Perform one cycle's worth of preparations for an operation. This method is
  // non-blocking, so should be executed repeatedly until preconditionsMet()
  // returns true.
  virtual void prepare() = 0;

  // Returns whether the operation is ready to begin.
  virtual bool preconditionsMet() const = 0;

  // Perform one cycle's worth of execution of the operation. This method is
  // non-blocking, so should be executed repeatedly until complete() returns
  // true.
  virtual void execute() = 0;

  // Returns whether the operation has completed.
  virtual bool complete() const;

  // Returns whether any further payloads are expected.
  virtual bool awaitingPayload() const;

  // Returns whether any further results are to be sent.
  virtual bool resultsToSend() const;

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

  // Return the NetworkRequest which generated this MemoryOperation.
  NetworkRequest getOriginal() const;

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

protected:

  // Perform safety checks before writing any data.
  void preWriteCheck() const;

  // Returns whether the memory should be treated as a cache or scratchpad.
  MemoryAccessMode getAccessMode() const;

  // Returns whether the memory has a payload flit ready for this operation.
  bool payloadAvailable() const;

  // Retrieves a payload flit from the memory.
  unsigned int getPayload();

  // Send a word to the chosen destination.
  void sendResult(unsigned int data);

  // Compute the address of the start of the cache line which contains the given
  // address.
  MemoryAddr startOfLine(MemoryAddr address) const;

protected:

  const MemoryAddr      address;        // Position in address space
  const MemoryMetadata  metadata;       // Various information modifying the operation
  MemoryInterface&      memory;         // The memory processing this operation
  const MemoryLevel     level;          // Whether this is the L1, L2, etc.
  const ChannelID       destination;    // Channel to send results to
  unsigned int          payloadFlits;   // Number of payload flits remaining
  unsigned int          resultFlits;    // Number of result flits remaining

  SRAMAddress           sramAddress;    // Position in SRAM
};

#endif /* SRC_TILECOMPONENTS_MEMORY_OPERATIONS_MEMORYOPERATION_H_ */
