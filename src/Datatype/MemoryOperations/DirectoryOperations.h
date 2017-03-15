/*
 * DirectoryOperations.h
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORY_OPERATIONS_DIRECTORYOPERATIONS_H_
#define SRC_TILE_MEMORY_OPERATIONS_DIRECTORYOPERATIONS_H_

#include "../Flit.h"
#include "../Identifier.h"
#include "../../Tile/Memory/MemoryTypedefs.h"
#include "MemoryOperation.h"

class UpdateDirectoryEntry : public MemoryOperation {
public:
  UpdateDirectoryEntry(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual bool needsForwarding() const;
  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

class UpdateDirectoryMask : public MemoryOperation {
public:
  UpdateDirectoryMask(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination);

  virtual bool needsForwarding() const;
  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
};

#endif /* SRC_TILE_MEMORY_OPERATIONS_DIRECTORYOPERATIONS_H_ */
