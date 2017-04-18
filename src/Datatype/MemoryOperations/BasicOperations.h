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

class LoadWord : public MemoryOperation {
public:
  LoadWord(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class LoadHalfword : public MemoryOperation {
public:
  LoadHalfword(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class LoadByte : public MemoryOperation {
public:
  LoadByte(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class StoreWord : public MemoryOperation {
public:
  StoreWord(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class StoreHalfword : public MemoryOperation {
public:
  StoreHalfword(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class StoreByte : public MemoryOperation {
public:
  StoreByte(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

#endif /* SRC_TILE_MEMORY_OPERATIONS_BASICOPERATIONS_H_ */
