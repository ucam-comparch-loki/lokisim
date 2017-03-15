/*
 * MemoryOperationDecode.h
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORY_OPERATIONS_MEMORYOPERATIONDECODE_H_
#define SRC_TILE_MEMORY_OPERATIONS_MEMORYOPERATIONDECODE_H_

#include "../../Exceptions/InvalidOptionException.h"
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

  switch (metadata.opcode) {

    case LOAD_W:
      return new LoadWord(request, memory, level, destination);
    case LOAD_LINKED:
      return new LoadLinked(request, memory, level, destination);
    case LOAD_HW:
      return new LoadHalfword(request, memory, level, destination);
    case LOAD_B:
      return new LoadByte(request, memory, level, destination);
    case FETCH_LINE:
      return new FetchLine(request, memory, level, destination);
    case IPK_READ:
      return new IPKRead(request, memory, level, destination);
    case VALIDATE_LINE:
      return new ValidateLine(request, memory, level, destination);
    case PREFETCH_LINE:
      return new PrefetchLine(request, memory, level, destination);
    case FLUSH_LINE:
      return new FlushLine(request, memory, level, destination);
    case INVALIDATE_LINE:
      return new InvalidateLine(request, memory, level, destination);
    case FLUSH_ALL_LINES:
      return new FlushAllLines(request, memory, level, destination);
    case INVALIDATE_ALL_LINES:
      return new InvalidateAllLines(request, memory, level, destination);
    case STORE_W:
      return new StoreWord(request, memory, level, destination);
    case STORE_CONDITIONAL:
      return new StoreConditional(request, memory, level, destination);
    case STORE_HW:
      return new StoreHalfword(request, memory, level, destination);
    case STORE_B:
      return new StoreByte(request, memory, level, destination);
    case STORE_LINE:
      return new StoreLine(request, memory, level, destination);
    case MEMSET_LINE:
      return new MemsetLine(request, memory, level, destination);
    case PUSH_LINE:
      return new PushLine(request, memory, level, destination);
    case LOAD_AND_ADD:
      return new LoadAndAdd(request, memory, level, destination);
    case LOAD_AND_OR:
      return new LoadAndOr(request, memory, level, destination);
    case LOAD_AND_AND:
      return new LoadAndAnd(request, memory, level, destination);
    case LOAD_AND_XOR:
      return new LoadAndXor(request, memory, level, destination);
    case EXCHANGE:
      return new Exchange(request, memory, level, destination);
    case UPDATE_DIRECTORY_ENTRY:
      return new UpdateDirectoryEntry(request, memory, level, destination);
    case UPDATE_DIRECTORY_MASK:
      return new UpdateDirectoryMask(request, memory, level, destination);

    default:
      throw InvalidOptionException("memory request opcode", (int)metadata.opcode);
      return NULL;

  }

}



#endif /* SRC_TILE_MEMORY_OPERATIONS_MEMORYOPERATIONDECODE_H_ */
