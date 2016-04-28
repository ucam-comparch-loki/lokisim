/*
 * BasicOperations.h
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#ifndef SRC_TILECOMPONENTS_MEMORY_OPERATIONS_BASICOPERATIONS_H_
#define SRC_TILECOMPONENTS_MEMORY_OPERATIONS_BASICOPERATIONS_H_

#include "../../../Datatype/Flit.h"
#include "../../../Datatype/Identifier.h"
#include "../MemoryTypedefs.h"
#include "MemoryOperation.h"

class LoadWord : public MemoryOperation {
public:
  LoadWord(MemoryAddr address, MemoryMetadata metadata, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class LoadHalfword : public MemoryOperation {
public:
  LoadHalfword(MemoryAddr address, MemoryMetadata metadata, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class LoadByte : public MemoryOperation {
public:
  LoadByte(MemoryAddr address, MemoryMetadata metadata, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class StoreWord : public MemoryOperation {
public:
  StoreWord(MemoryAddr address, MemoryMetadata metadata, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class StoreHalfword : public MemoryOperation {
public:
  StoreHalfword(MemoryAddr address, MemoryMetadata metadata, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class StoreByte : public MemoryOperation {
public:
  StoreByte(MemoryAddr address, MemoryMetadata metadata, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

#endif /* SRC_TILECOMPONENTS_MEMORY_OPERATIONS_BASICOPERATIONS_H_ */
