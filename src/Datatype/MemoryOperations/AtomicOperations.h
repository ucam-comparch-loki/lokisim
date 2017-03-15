/*
 * AtomicOperations.h
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORY_OPERATIONS_ATOMICOPERATIONS_H_
#define SRC_TILE_MEMORY_OPERATIONS_ATOMICOPERATIONS_H_

#include "../Flit.h"
#include "../Identifier.h"
#include "../../Tile/Memory/MemoryTypedefs.h"
#include "MemoryOperation.h"

class LoadLinked : public MemoryOperation {
public:
  LoadLinked(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class StoreConditional: public MemoryOperation {
public:
  StoreConditional(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();

private:
  bool success; // Whether the operation should proceed.
};

class LoadAndAdd : public MemoryOperation {
public:
  LoadAndAdd(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();

private:
  unsigned int intermediateData;  // The data to be modified and stored back.
};

class LoadAndOr : public MemoryOperation {
public:
  LoadAndOr(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();

private:
  unsigned int intermediateData;  // The data to be modified and stored back.
};

class LoadAndAnd : public MemoryOperation {
public:
  LoadAndAnd(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();

private:
  unsigned int intermediateData;  // The data to be modified and stored back.
};

class LoadAndXor : public MemoryOperation {
public:
  LoadAndXor(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();

private:
  unsigned int intermediateData;  // The data to be modified and stored back.
};

class Exchange : public MemoryOperation {
public:
  Exchange(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

#endif /* SRC_TILE_MEMORY_OPERATIONS_ATOMICOPERATIONS_H_ */
