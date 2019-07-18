/*
 * CacheLineOperations.h
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORY_OPERATIONS_CACHELINEOPERATIONS_H_
#define SRC_TILE_MEMORY_OPERATIONS_CACHELINEOPERATIONS_H_

#include "../../Memory/MemoryTypes.h"
#include "../Flit.h"
#include "../Identifier.h"
#include "BasicOperations.h"
#include "MemoryOperation.h"

class MetadataOperation : public MemoryOperation {
public:
  MetadataOperation(const NetworkRequest& request, ChannelID destination);
  virtual uint payloadFlitsRemaining() const;
  virtual uint resultFlitsRemaining() const;
};


class FetchLine : public LoadOperation {
public:
  FetchLine(const NetworkRequest& request, ChannelID destination);
};

class IPKRead : public LoadOperation {
public:
  IPKRead(const NetworkRequest& request, ChannelID destination);
};

class ValidateLine : public MetadataOperation {
public:
  ValidateLine(const NetworkRequest& request, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class PrefetchLine : public LoadOperation {
public:
  PrefetchLine(const NetworkRequest& request, ChannelID destination);
  virtual void execute();
  virtual bool complete() const;
};

class FlushLine : public MetadataOperation {
public:
  FlushLine(const NetworkRequest& request, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;

private:
  bool finished;      // Whether the operation has completed.
};

class InvalidateLine : public MetadataOperation {
public:
  InvalidateLine(const NetworkRequest& request, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;

private:
  bool finished;      // Whether the operation has completed.
};

class FlushAllLines : public MetadataOperation {
public:
  FlushAllLines(const NetworkRequest& request, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;

private:
  uint line;          // The line currently being flushed.
};

class InvalidateAllLines : public MetadataOperation {
public:
  InvalidateAllLines(const NetworkRequest& request, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;

private:
  uint line;          // The line currently being invalidated.
};

class StoreLine : public StoreOperation {
public:
  StoreLine(const NetworkRequest& request, ChannelID destination);
};

class MemsetLine : public StoreLine {
public:
  MemsetLine(const NetworkRequest& request, ChannelID destination);

protected:
  // Payload methods are overridden so we only get one payload, and then reuse
  // it every iteration.
  bool payloadAvailable() const;
  unsigned int getPayload();

private:
  unsigned int data;  // The word to store to each position in the cache line.
  bool haveData;      // Have we received data yet?
};

class PushLine : public StoreLine {
public:
  PushLine(const NetworkRequest& request, ChannelID destination);
  virtual NetworkRequest toFlit() const;

private:
  unsigned int targetBank; // Bank in the remote tile to receive data.
};

#endif /* SRC_TILE_MEMORY_OPERATIONS_CACHELINEOPERATIONS_H_ */
