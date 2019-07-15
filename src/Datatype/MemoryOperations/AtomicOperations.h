/*
 * AtomicOperations.h
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORY_OPERATIONS_ATOMICOPERATIONS_H_
#define SRC_TILE_MEMORY_OPERATIONS_ATOMICOPERATIONS_H_

#include "../../Memory/MemoryTypes.h"
#include "../Flit.h"
#include "../Identifier.h"
#include "MemoryOperation.h"
#include "BasicOperations.h"

class AtomicOperation : public LoadStoreOperation {
public:
  AtomicOperation(const NetworkRequest& request, MemoryBase& memory,
                  MemoryLevel level, ChannelID destination);
  virtual void execute();

protected:
  // The update to be applied to the data in memory. To be implemented by every
  // subclass.
  virtual uint atomicUpdate(uint original, uint update) = 0;

private:
  unsigned int intermediateData;  // The data to be modified and stored back.
};


class LoadLinked : public LoadWord {
public:
  LoadLinked(const NetworkRequest& request, MemoryBase& memory,
             MemoryLevel level, ChannelID destination);
  virtual void execute();
};


class StoreConditional: public LoadStoreOperation {
public:
  StoreConditional(const NetworkRequest& request, MemoryBase& memory,
                   MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();

private:
  bool success; // Whether the operation should proceed.
};


class LoadAndAdd : public AtomicOperation {
public:
  LoadAndAdd(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);
protected:
  virtual uint atomicUpdate(uint original, uint update);
};

class LoadAndOr : public AtomicOperation {
public:
  LoadAndOr(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);
protected:
  virtual uint atomicUpdate(uint original, uint update);
};

class LoadAndAnd : public AtomicOperation {
public:
  LoadAndAnd(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);
protected:
  virtual uint atomicUpdate(uint original, uint update);
};

class LoadAndXor : public AtomicOperation {
public:
  LoadAndXor(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);
protected:
  virtual uint atomicUpdate(uint original, uint update);
};

class Exchange : public AtomicOperation {
public:
  Exchange(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);
protected:
  virtual uint atomicUpdate(uint original, uint update);
};

#endif /* SRC_TILE_MEMORY_OPERATIONS_ATOMICOPERATIONS_H_ */
