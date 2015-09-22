/*
 * CacheLineOperations.h
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#ifndef SRC_TILECOMPONENTS_MEMORY_OPERATIONS_CACHELINEOPERATIONS_H_
#define SRC_TILECOMPONENTS_MEMORY_OPERATIONS_CACHELINEOPERATIONS_H_

#include "../../../Datatype/Flit.h"
#include "../../../Datatype/Identifier.h"
#include "../MemoryTypedefs.h"
#include "MemoryOperation.h"

class FetchLine : public MemoryOperation {
public:
  FetchLine(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class IPKRead : public MemoryOperation {
public:
  IPKRead(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();

private:
  unsigned int lineCursor;
};

class ValidateLine : public MemoryOperation {
public:
  ValidateLine(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class PrefetchLine : public MemoryOperation {
public:
  PrefetchLine(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class FlushLine : public MemoryOperation {
public:
  FlushLine(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;

private:
  bool finished;      // Whether the operation has completed.
};

class InvalidateLine : public MemoryOperation {
public:
  InvalidateLine(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;

private:
  bool finished;      // Whether the operation has completed.
};

class FlushAllLines : public MemoryOperation {
public:
  FlushAllLines(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;
};

class InvalidateAllLines : public MemoryOperation {
public:
  InvalidateAllLines(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;
};

class StoreLine : public MemoryOperation {
public:
  StoreLine(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class MemsetLine : public MemoryOperation {
public:
  MemsetLine(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual bool complete() const;

private:
  unsigned int data;  // The word to store to each position in the cache line.
  bool finished;      // Whether the operation has completed.
};

class PushLine : public MemoryOperation {
public:
  PushLine(MemoryAddr address, MemoryMetadata metadata, MemoryBank& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

#endif /* SRC_TILECOMPONENTS_MEMORY_OPERATIONS_CACHELINEOPERATIONS_H_ */
