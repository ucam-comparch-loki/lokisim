/*
 * BasicOperations.h
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORY_OPERATIONS_BASICOPERATIONS_H_
#define SRC_TILE_MEMORY_OPERATIONS_BASICOPERATIONS_H_

#include "../../Memory/MemoryTypes.h"
#include "../Flit.h"
#include "../Identifier.h"
#include "MemoryOperation.h"

class LoadStoreOperation : public MemoryOperation {
public:
  LoadStoreOperation(MemoryAddr address,       // Address to access
                     MemoryMetadata metadata,  // Cache/scratchpad, skip, etc.
                     ChannelID returnAddress,  // Network destination for results
                     MemoryData datatype,      // Type of data to be accessed
                     MemoryAlignment alignment,// Alignment of initial address
                     uint iterations,          // Consecutive data to access
                     bool reads,               // Does this operation read memory?
                     bool writes);             // Does this operation write memory?

  virtual bool preconditionsMet() const;
};

class LoadOperation : public LoadStoreOperation {
public:
  LoadOperation(MemoryAddr address, MemoryMetadata metadata,
                ChannelID returnAddress, MemoryData datatype,
                MemoryAlignment alignment, uint iterations);

  virtual uint payloadFlitsRemaining() const;
  virtual uint resultFlitsRemaining() const;

protected:
  virtual bool oneIteration();
};

class StoreOperation : public LoadStoreOperation {
public:
  StoreOperation(MemoryAddr address, MemoryMetadata metadata,
                 ChannelID returnAddress, MemoryData datatype,
                 MemoryAlignment alignment, uint iterations);

  virtual uint payloadFlitsRemaining() const;
  virtual uint resultFlitsRemaining() const;

protected:
  virtual bool oneIteration();
};

class LoadWord : public LoadOperation {
public:
  LoadWord(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
};

class LoadHalfword : public LoadOperation {
public:
  LoadHalfword(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
};

class LoadByte : public LoadOperation {
public:
  LoadByte(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
};

class StoreWord : public StoreOperation {
public:
  StoreWord(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
};

class StoreHalfword : public StoreOperation {
public:
  StoreHalfword(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
};

class StoreByte : public StoreOperation {
public:
  StoreByte(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
};

#endif /* SRC_TILE_MEMORY_OPERATIONS_BASICOPERATIONS_H_ */
