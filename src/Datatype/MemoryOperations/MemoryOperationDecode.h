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
  MemoryOperation* op;

  switch (metadata.opcode) {

    case LOAD_W:
      op = new LoadWord(request, destination); break;
    case LOAD_LINKED:
      op = new LoadLinked(request, destination); break;
    case LOAD_HW:
      op = new LoadHalfword(request, destination); break;
    case LOAD_B:
      op = new LoadByte(request, destination); break;
    case FETCH_LINE:
      op = new FetchLine(request, destination); break;
    case IPK_READ:
      op = new IPKRead(request, destination); break;
    case VALIDATE_LINE:
      op = new ValidateLine(request, destination); break;
    case PREFETCH_LINE:
      op = new PrefetchLine(request, destination); break;
    case FLUSH_LINE:
      op = new FlushLine(request, destination); break;
    case INVALIDATE_LINE:
      op = new InvalidateLine(request, destination); break;
    case FLUSH_ALL_LINES:
      op = new FlushAllLines(request, destination); break;
    case INVALIDATE_ALL_LINES:
      op = new InvalidateAllLines(request, destination); break;
    case STORE_W:
      op = new StoreWord(request, destination); break;
    case STORE_CONDITIONAL:
      op = new StoreConditional(request, destination); break;
    case STORE_HW:
      op = new StoreHalfword(request, destination); break;
    case STORE_B:
      op = new StoreByte(request, destination); break;
    case STORE_LINE:
      op = new StoreLine(request, destination); break;
    case MEMSET_LINE:
      op = new MemsetLine(request, destination); break;
    case PUSH_LINE:
      op = new PushLine(request, destination); break;
    case LOAD_AND_ADD:
      op = new LoadAndAdd(request, destination); break;
    case LOAD_AND_OR:
      op = new LoadAndOr(request, destination); break;
    case LOAD_AND_AND:
      op = new LoadAndAnd(request, destination); break;
    case LOAD_AND_XOR:
      op = new LoadAndXor(request, destination); break;
    case EXCHANGE:
      op = new Exchange(request, destination); break;
    case UPDATE_DIRECTORY_ENTRY:
      op = new UpdateDirectoryEntry(request, destination); break;
    case UPDATE_DIRECTORY_MASK:
      op = new UpdateDirectoryMask(request, destination); break;

    default:
      throw InvalidOptionException("memory request opcode", (int)metadata.opcode);
      op = NULL;
      break;

  }

  op->assignToMemory(memory, level);
  return op;

}



#endif /* SRC_TILE_MEMORY_OPERATIONS_MEMORYOPERATIONDECODE_H_ */
