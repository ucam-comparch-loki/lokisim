/*
 * AtomicOperations.h
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#ifndef SRC_TILECOMPONENTS_MEMORY_OPERATIONS_ATOMICOPERATIONS_H_
#define SRC_TILECOMPONENTS_MEMORY_OPERATIONS_ATOMICOPERATIONS_H_

#include "../../../Datatype/Flit.h"
#include "../../../Datatype/Identifier.h"
#include "../MemoryTypedefs.h"
#include "MemoryOperation.h"

class LoadLinked : public MemoryOperation {
public:
  LoadLinked(MemoryAddr address, MemoryMetadata metadata, MemoryInterface& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class StoreConditional: public MemoryOperation {
public:
  StoreConditional(MemoryAddr address, MemoryMetadata metadata, MemoryInterface& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();

private:
  bool success; // Whether the operation should proceed.
};

class LoadAndAdd : public MemoryOperation {
public:
  LoadAndAdd(MemoryAddr address, MemoryMetadata metadata, MemoryInterface& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();

private:
  unsigned int intermediateData;  // The data to be modified and stored back.
};

class LoadAndOr : public MemoryOperation {
public:
  LoadAndOr(MemoryAddr address, MemoryMetadata metadata, MemoryInterface& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();

private:
  unsigned int intermediateData;  // The data to be modified and stored back.
};

class LoadAndAnd : public MemoryOperation {
public:
  LoadAndAnd(MemoryAddr address, MemoryMetadata metadata, MemoryInterface& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();

private:
  unsigned int intermediateData;  // The data to be modified and stored back.
};

class LoadAndXor : public MemoryOperation {
public:
  LoadAndXor(MemoryAddr address, MemoryMetadata metadata, MemoryInterface& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();

private:
  unsigned int intermediateData;  // The data to be modified and stored back.
};

class Exchange : public MemoryOperation {
public:
  Exchange(MemoryAddr address, MemoryMetadata metadata, MemoryInterface& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

#endif /* SRC_TILECOMPONENTS_MEMORY_OPERATIONS_ATOMICOPERATIONS_H_ */
