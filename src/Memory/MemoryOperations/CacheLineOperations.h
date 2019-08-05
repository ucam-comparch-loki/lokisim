/*
 * CacheLineOperations.h
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORY_OPERATIONS_CACHELINEOPERATIONS_H_
#define SRC_TILE_MEMORY_OPERATIONS_CACHELINEOPERATIONS_H_

#include "../MemoryTypes.h"
#include "../../Datatype/Flit.h"
#include "../../Datatype/Identifier.h"
#include "BasicOperations.h"
#include "MemoryOperation.h"

// Operation which only works on metadata (e.g. valid, dirty, ...).
class MetadataOperation : public MemoryOperation {
public:
  MetadataOperation(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
  virtual uint payloadFlitsRemaining() const;
  virtual uint resultFlitsRemaining() const;
};

// Operation which iterates over all cache lines.
class AllLinesOperation : public MemoryOperation {
public:
  AllLinesOperation(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
  virtual void assignToMemory(MemoryBase& memory, MemoryLevel level);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual uint payloadFlitsRemaining() const;
  virtual uint resultFlitsRemaining() const;
};


class FetchLine : public LoadOperation {
public:
  FetchLine(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
};

class IPKRead : public LoadOperation {
public:
  IPKRead(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
};

class ValidateLine : public MetadataOperation {
public:
  ValidateLine(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class PrefetchLine : public LoadOperation {
public:
  PrefetchLine(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
  virtual void execute();
  virtual bool complete() const;
};

class FlushLine : public MetadataOperation {
public:
  FlushLine(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;

private:
  bool finished;      // Whether the operation has completed.
};

class InvalidateLine : public MetadataOperation {
public:
  InvalidateLine(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;

private:
  bool finished;      // Whether the operation has completed.
};

class FlushAllLines : public AllLinesOperation {
public:
  FlushAllLines(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
protected:
  virtual bool oneIteration();
};

class InvalidateAllLines : public AllLinesOperation {
public:
  InvalidateAllLines(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
protected:
  virtual bool oneIteration();
};

class StoreLine : public StoreOperation {
public:
  StoreLine(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
};

class MemsetLine : public StoreLine {
public:
  MemsetLine(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);

protected:
  // Payload methods are overridden so we only get one payload, and then reuse
  // it every iteration.
  virtual bool payloadAvailable() const;
  virtual unsigned int getPayload();

private:
  unsigned int data;  // The word to store to each position in the cache line.
  bool haveData;      // Have we received data yet?
};

class PushLine : public StoreLine {
public:
  PushLine(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
  virtual void assignToMemory(MemoryBase& memory, MemoryLevel level);
  virtual NetworkRequest toFlit() const;

private:
  unsigned int targetBank; // Bank in the remote tile to receive data.
};

#endif /* SRC_TILE_MEMORY_OPERATIONS_CACHELINEOPERATIONS_H_ */
