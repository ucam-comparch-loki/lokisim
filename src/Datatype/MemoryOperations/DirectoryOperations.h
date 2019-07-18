/*
 * DirectoryOperations.h
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORY_OPERATIONS_DIRECTORYOPERATIONS_H_
#define SRC_TILE_MEMORY_OPERATIONS_DIRECTORYOPERATIONS_H_

#include "../../Memory/MemoryTypes.h"
#include "../Flit.h"
#include "../Identifier.h"
#include "MemoryOperation.h"

class DirectoryOperation : public MemoryOperation {
public:
  DirectoryOperation(const NetworkRequest& request, ChannelID destination,
                     unsigned int payloadFlits);

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
  UpdateDirectoryEntry(const NetworkRequest& request, ChannelID destination);
};

class UpdateDirectoryMask : public DirectoryOperation {
public:
  UpdateDirectoryMask(const NetworkRequest& request, ChannelID destination);
};

#endif /* SRC_TILE_MEMORY_OPERATIONS_DIRECTORYOPERATIONS_H_ */
