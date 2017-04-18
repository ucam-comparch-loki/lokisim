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

class FetchLine : public MemoryOperation {
public:
  FetchLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();

private:
  unsigned int lineCursor;
};

class IPKRead : public MemoryOperation {
public:
  IPKRead(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();

private:
  unsigned int lineCursor;
};

class ValidateLine : public MemoryOperation {
public:
  ValidateLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class PrefetchLine : public MemoryOperation {
public:
  PrefetchLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class FlushLine : public MemoryOperation {
public:
  FlushLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;

private:
  bool finished;      // Whether the operation has completed.
};

class InvalidateLine : public MemoryOperation {
public:
  InvalidateLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;

private:
  bool finished;      // Whether the operation has completed.
};

class FlushAllLines : public MemoryOperation {
public:
  FlushAllLines(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;

private:
  uint line;          // The line currently being flushed.
};

class InvalidateAllLines : public MemoryOperation {
public:
  InvalidateAllLines(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;

private:
  uint line;          // The line currently being invalidated.
};

class StoreLine : public MemoryOperation {
public:
  StoreLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();

private:
  unsigned int lineCursor;
};

class MemsetLine : public MemoryOperation {
public:
  MemsetLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;

private:
  unsigned int lineCursor;
  unsigned int data;  // The word to store to each position in the cache line.
};

class PushLine : public MemoryOperation {
public:
  PushLine(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual const NetworkRequest getOriginal() const;

private:
  unsigned int lineCursor;
  unsigned int targetBank; // Bank in the remote tile to receive data.
};

#endif /* SRC_TILE_MEMORY_OPERATIONS_CACHELINEOPERATIONS_H_ */
