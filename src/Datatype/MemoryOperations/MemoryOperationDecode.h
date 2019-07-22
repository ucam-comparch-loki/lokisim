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
                                            const ChannelID returnAddress) {

  MemoryAddr address = request.payload().toUInt();
  MemoryMetadata metadata = request.getMemoryMetadata();
  MemoryOperation* op;

  switch (metadata.opcode) {

    case LOAD_W:
      op = new LoadWord(address, metadata, returnAddress); break;
    case LOAD_LINKED:
      op = new LoadLinked(address, metadata, returnAddress); break;
    case LOAD_HW:
      op = new LoadHalfword(address, metadata, returnAddress); break;
    case LOAD_B:
      op = new LoadByte(address, metadata, returnAddress); break;
    case FETCH_LINE:
      op = new FetchLine(address, metadata, returnAddress); break;
    case IPK_READ:
      op = new IPKRead(address, metadata, returnAddress); break;
    case VALIDATE_LINE:
      op = new ValidateLine(address, metadata, returnAddress); break;
    case PREFETCH_LINE:
      op = new PrefetchLine(address, metadata, returnAddress); break;
    case FLUSH_LINE:
      op = new FlushLine(address, metadata, returnAddress); break;
    case INVALIDATE_LINE:
      op = new InvalidateLine(address, metadata, returnAddress); break;
    case FLUSH_ALL_LINES:
      op = new FlushAllLines(address, metadata, returnAddress); break;
    case INVALIDATE_ALL_LINES:
      op = new InvalidateAllLines(address, metadata, returnAddress); break;
    case STORE_W:
      op = new StoreWord(address, metadata, returnAddress); break;
    case STORE_CONDITIONAL:
      op = new StoreConditional(address, metadata, returnAddress); break;
    case STORE_HW:
      op = new StoreHalfword(address, metadata, returnAddress); break;
    case STORE_B:
      op = new StoreByte(address, metadata, returnAddress); break;
    case STORE_LINE:
      op = new StoreLine(address, metadata, returnAddress); break;
    case MEMSET_LINE:
      op = new MemsetLine(address, metadata, returnAddress); break;
    case PUSH_LINE:
      op = new PushLine(address, metadata, returnAddress); break;
    case LOAD_AND_ADD:
      op = new LoadAndAdd(address, metadata, returnAddress); break;
    case LOAD_AND_OR:
      op = new LoadAndOr(address, metadata, returnAddress); break;
    case LOAD_AND_AND:
      op = new LoadAndAnd(address, metadata, returnAddress); break;
    case LOAD_AND_XOR:
      op = new LoadAndXor(address, metadata, returnAddress); break;
    case EXCHANGE:
      op = new Exchange(address, metadata, returnAddress); break;
    case UPDATE_DIRECTORY_ENTRY:
      op = new UpdateDirectoryEntry(address, metadata, returnAddress); break;
    case UPDATE_DIRECTORY_MASK:
      op = new UpdateDirectoryMask(address, metadata, returnAddress); break;

    default:
      throw InvalidOptionException("memory request opcode", (int)metadata.opcode);
      op = NULL;
      break;

  }

  op->assignToMemory(memory, level);
  return op;

}



#endif /* SRC_TILE_MEMORY_OPERATIONS_MEMORYOPERATIONDECODE_H_ */
