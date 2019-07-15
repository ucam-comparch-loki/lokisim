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
#include "MemoryOperation.h"

class CacheLineOperation : public MemoryOperation {
public:
  CacheLineOperation(const NetworkRequest& request,
                     MemoryBase& memory,
                     MemoryLevel level,
                     ChannelID destination,
                     unsigned int payloadFlits,
                     unsigned int maxResultFlits,
                     unsigned int alignment);
protected:
  unsigned int lineCursor;
};

class ReadLineOperation : public CacheLineOperation {
public:
  ReadLineOperation(const NetworkRequest& request,
                    MemoryBase& memory,
                    MemoryLevel level,
                    ChannelID destination,
                    unsigned int alignment);

  virtual void prepare();
  virtual bool preconditionsMet() const;
};

class WriteLineOperation : public CacheLineOperation {
public:
  WriteLineOperation(const NetworkRequest& request,
                    MemoryBase& memory,
                    MemoryLevel level,
                    ChannelID destination,
                    unsigned int payloadFlits);

  virtual void prepare();
  virtual bool preconditionsMet() const;
};

class MetadataOperation : public MemoryOperation {
public:
  MetadataOperation(const NetworkRequest& request,
                    MemoryBase& memory,
                    MemoryLevel level,
                    ChannelID destination);
};


class FetchLine : public ReadLineOperation {
public:
  FetchLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);
  virtual void execute();
};

class IPKRead : public ReadLineOperation {
public:
  IPKRead(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);
  virtual void execute();
};

class ValidateLine : public MetadataOperation {
public:
  ValidateLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class PrefetchLine : public ReadLineOperation {
public:
  PrefetchLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);
  virtual void execute();
  virtual bool complete() const;
};

class FlushLine : public MetadataOperation {
public:
  FlushLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;

private:
  bool finished;      // Whether the operation has completed.
};

class InvalidateLine : public MetadataOperation {
public:
  InvalidateLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;

private:
  bool finished;      // Whether the operation has completed.
};

class FlushAllLines : public MetadataOperation {
public:
  FlushAllLines(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;

private:
  uint line;          // The line currently being flushed.
};

class InvalidateAllLines : public MetadataOperation {
public:
  InvalidateAllLines(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;

private:
  uint line;          // The line currently being invalidated.
};

class StoreLine : public WriteLineOperation {
public:
  StoreLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);
  virtual void execute();
};

class MemsetLine : public WriteLineOperation {
public:
  MemsetLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);
  virtual void execute();
  virtual bool complete() const;

private:
  unsigned int data;  // The word to store to each position in the cache line.
};

class PushLine : public WriteLineOperation {
public:
  PushLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);
  virtual void execute();
  virtual const NetworkRequest getOriginal() const;

private:
  unsigned int targetBank; // Bank in the remote tile to receive data.
};

#endif /* SRC_TILE_MEMORY_OPERATIONS_CACHELINEOPERATIONS_H_ */
