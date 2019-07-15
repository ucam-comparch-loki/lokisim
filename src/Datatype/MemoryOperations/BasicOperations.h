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

class LoadStoreOperation : public MemoryOperation {
public:
  LoadStoreOperation(const NetworkRequest& request,
                     MemoryBase& memory,
                     MemoryLevel level,
                     ChannelID destination,
                     unsigned int payloadFlits,
                     unsigned int maxResultFlits,
                     unsigned int alignment);

  virtual void prepare();
  virtual bool preconditionsMet() const;
};

class LoadOperation : public LoadStoreOperation {
public:
  LoadOperation(const NetworkRequest& request, MemoryBase& memory,
                MemoryLevel level, ChannelID destination,
                unsigned int alignment);
};

class StoreOperation : public LoadStoreOperation {
public:
  StoreOperation(const NetworkRequest& request, MemoryBase& memory,
                 MemoryLevel level, ChannelID destination,
                 unsigned int alignment);
};

class LoadWord : public LoadOperation {
public:
  LoadWord(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);
  virtual void execute();
};

class LoadHalfword : public LoadOperation {
public:
  LoadHalfword(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);
  virtual void execute();
};

class LoadByte : public LoadOperation {
public:
  LoadByte(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);
  virtual void execute();
};

class StoreWord : public StoreOperation {
public:
  StoreWord(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);
  virtual void execute();
};

class StoreHalfword : public StoreOperation {
public:
  StoreHalfword(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);
  virtual void execute();
};

class StoreByte : public StoreOperation {
public:
  StoreByte(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);
  virtual void execute();
};

#endif /* SRC_TILE_MEMORY_OPERATIONS_BASICOPERATIONS_H_ */
