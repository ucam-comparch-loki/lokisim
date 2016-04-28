/*
 * MemoryOperationDecode.h
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#ifndef SRC_TILECOMPONENTS_MEMORY_OPERATIONS_MEMORYOPERATIONDECODE_H_
#define SRC_TILECOMPONENTS_MEMORY_OPERATIONS_MEMORYOPERATIONDECODE_H_

#include "../../../Exceptions/InvalidOptionException.h"
#include "AtomicOperations.h"
#include "BasicOperations.h"
#include "CacheLineOperations.h"
#include "DirectoryOperations.h"
#include "MemoryOperation.h"

inline MemoryOperation* decodeMemoryRequest(const NetworkRequest& request,
                                            MemoryBase& memory,
                                            const MemoryLevel level,
                                            const ChannelID destination) {

  MemoryMetadata metadata = request.getMemoryMetadata();
  MemoryAddr address = request.payload().toUInt();

  switch (metadata.opcode) {

    case LOAD_W:
      return new LoadWord(address, metadata, memory, level, destination);
    case LOAD_LINKED:
      return new LoadLinked(address, metadata, memory, level, destination);
    case LOAD_HW:
      return new LoadHalfword(address, metadata, memory, level, destination);
    case LOAD_B:
      return new LoadByte(address, metadata, memory, level, destination);
    case FETCH_LINE:
      return new FetchLine(address, metadata, memory, level, destination);
    case IPK_READ:
      return new IPKRead(address, metadata, memory, level, destination);
    case VALIDATE_LINE:
      return new ValidateLine(address, metadata, memory, level, destination);
    case PREFETCH_LINE:
      return new PrefetchLine(address, metadata, memory, level, destination);
    case FLUSH_LINE:
      return new FlushLine(address, metadata, memory, level, destination);
    case INVALIDATE_LINE:
      return new InvalidateLine(address, metadata, memory, level, destination);
    case FLUSH_ALL_LINES:
      return new FlushAllLines(address, metadata, memory, level, destination);
    case INVALIDATE_ALL_LINES:
      return new InvalidateAllLines(address, metadata, memory, level, destination);
    case STORE_W:
      return new StoreWord(address, metadata, memory, level, destination);
    case STORE_CONDITIONAL:
      return new StoreConditional(address, metadata, memory, level, destination);
    case STORE_HW:
      return new StoreHalfword(address, metadata, memory, level, destination);
    case STORE_B:
      return new StoreByte(address, metadata, memory, level, destination);
    case STORE_LINE:
      return new StoreLine(address, metadata, memory, level, destination);
    case MEMSET_LINE:
      return new MemsetLine(address, metadata, memory, level, destination);
    case PUSH_LINE:
      return new PushLine(address, metadata, memory, level, destination);
    case LOAD_AND_ADD:
      return new LoadAndAdd(address, metadata, memory, level, destination);
    case LOAD_AND_OR:
      return new LoadAndOr(address, metadata, memory, level, destination);
    case LOAD_AND_AND:
      return new LoadAndAnd(address, metadata, memory, level, destination);
    case LOAD_AND_XOR:
      return new LoadAndXor(address, metadata, memory, level, destination);
    case EXCHANGE:
      return new Exchange(address, metadata, memory, level, destination);
    case UPDATE_DIRECTORY_ENTRY:
      return new UpdateDirectoryEntry(address, metadata, memory, level, destination);
    case UPDATE_DIRECTORY_MASK:
      return new UpdateDirectoryMask(address, metadata, memory, level, destination);

    default:
      throw InvalidOptionException("memory request opcode", (int)metadata.opcode);
      return NULL;

  }

}



#endif /* SRC_TILECOMPONENTS_MEMORY_OPERATIONS_MEMORYOPERATIONDECODE_H_ */
