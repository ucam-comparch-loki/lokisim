/*
 * DirectoryOperations.h
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORY_OPERATIONS_DIRECTORYOPERATIONS_H_
#define SRC_TILE_MEMORY_OPERATIONS_DIRECTORYOPERATIONS_H_

#include "../MemoryTypes.h"
#include "../../Datatype/Flit.h"
#include "../../Datatype/Identifier.h"
#include "MemoryOperation.h"

namespace Memory {

class DirectoryOperation : public MemoryOperation {
public:
  DirectoryOperation(MemoryAddr address, MemoryMetadata metadata,
                     ChannelID returnAddr, unsigned int payloadFlits);

  virtual bool needsForwarding() const;
  virtual void prepare();
  virtual bool preconditionsMet() const;
  virtual void execute();
  virtual uint payloadFlitsRemaining() const;
  virtual uint resultFlitsRemaining() const;
  virtual NetworkRequest toFlit() const;
};

class UpdateDirectoryEntry : public DirectoryOperation {
public:
  UpdateDirectoryEntry(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
};

class UpdateDirectoryMask : public DirectoryOperation {
public:
  UpdateDirectoryMask(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr);
};

} // end namespace

#endif /* SRC_TILE_MEMORY_OPERATIONS_DIRECTORYOPERATIONS_H_ */
