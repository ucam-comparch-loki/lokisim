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
  LoadWord(const NetworkRequest& request, ChannelID destination);
};

class LoadHalfword : public LoadOperation {
public:
  LoadHalfword(const NetworkRequest& request, ChannelID destination);
};

class LoadByte : public LoadOperation {
public:
  LoadByte(const NetworkRequest& request, ChannelID destination);
};

class StoreWord : public StoreOperation {
public:
  StoreWord(const NetworkRequest& request, ChannelID destination);
};

class StoreHalfword : public StoreOperation {
public:
  StoreHalfword(const NetworkRequest& request, ChannelID destination);
};

class StoreByte : public StoreOperation {
public:
  StoreByte(const NetworkRequest& request, ChannelID destination);
};

#endif /* SRC_TILE_MEMORY_OPERATIONS_BASICOPERATIONS_H_ */
