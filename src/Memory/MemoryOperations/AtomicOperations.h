/*
 * AtomicOperations.h
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORY_OPERATIONS_ATOMICOPERATIONS_H_
#define SRC_TILE_MEMORY_OPERATIONS_ATOMICOPERATIONS_H_

#include "../MemoryTypes.h"
#include "../../Datatype/Flit.h"
#include "../../Datatype/Identifier.h"
#include "MemoryOperation.h"
#include "BasicOperations.h"

namespace Memory {

class AtomicOperation : public LoadStoreOperation {
public:
  AtomicOperation(MemoryAddr address, MemoryMetadata metadata,
                  ChannelID returnAddress, MemoryData datatype,
                  MemoryAlignment alignment, uint iterations);

  virtual uint payloadFlitsRemaining() const;
  virtual uint resultFlitsRemaining() const;

protected:
  virtual bool oneIteration();

  // The update to be applied to the data in memory. To be implemented by every
  // subclass.
  virtual uint atomicUpdate(uint original, uint update) = 0;

private:
  uint intermediateData;  // The data to be modified and stored back.
  bool readState;         // Two states: reading or writing.
};


class LoadLinked : public LoadWord {
public:
  LoadLinked(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
protected:
  virtual bool oneIteration();
};


class StoreConditional: public LoadStoreOperation {
public:
  StoreConditional(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);

  virtual void prepare();
  virtual bool preconditionsMet() const;

  virtual uint payloadFlitsRemaining() const;
  virtual uint resultFlitsRemaining() const;

protected:
  virtual bool oneIteration();

private:
  bool success; // Whether the operation should proceed.
  bool checkState; // Two states: check reservation or write data
};


class LoadAndAdd : public AtomicOperation {
public:
  LoadAndAdd(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
protected:
  virtual uint atomicUpdate(uint original, uint update);
};

class LoadAndOr : public AtomicOperation {
public:
  LoadAndOr(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
protected:
  virtual uint atomicUpdate(uint original, uint update);
};

class LoadAndAnd : public AtomicOperation {
public:
  LoadAndAnd(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
protected:
  virtual uint atomicUpdate(uint original, uint update);
};

class LoadAndXor : public AtomicOperation {
public:
  LoadAndXor(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
protected:
  virtual uint atomicUpdate(uint original, uint update);
};

class Exchange : public AtomicOperation {
public:
  Exchange(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
protected:
  virtual uint atomicUpdate(uint original, uint update);
};

} // end namespace

#endif /* SRC_TILE_MEMORY_OPERATIONS_ATOMICOPERATIONS_H_ */
