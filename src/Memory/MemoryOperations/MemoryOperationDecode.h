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

inline Memory::MemoryOperation* decodeMemoryRequest(const NetworkRequest& request,
                                            MemoryBase& memory,
                                            const MemoryLevel level,
                                            const ChannelID returnAddress) {

  MemoryAddr address = request.payload().toUInt();
  MemoryMetadata metadata = request.getMemoryMetadata();
  Memory::MemoryOperation* op;

  switch (metadata.opcode) {

    case LOAD_W:
      op = new Memory::LoadWord(address, metadata, returnAddress); break;
    case LOAD_LINKED:
      op = new Memory::LoadLinked(address, metadata, returnAddress); break;
    case LOAD_HW:
      op = new Memory::LoadHalfword(address, metadata, returnAddress); break;
    case LOAD_B:
      op = new Memory::LoadByte(address, metadata, returnAddress); break;
    case FETCH_LINE:
      op = new Memory::FetchLine(address, metadata, returnAddress); break;
    case IPK_READ:
      op = new Memory::IPKRead(address, metadata, returnAddress); break;
    case VALIDATE_LINE:
      op = new Memory::ValidateLine(address, metadata, returnAddress); break;
    case PREFETCH_LINE:
      op = new Memory::PrefetchLine(address, metadata, returnAddress); break;
    case FLUSH_LINE:
      op = new Memory::FlushLine(address, metadata, returnAddress); break;
    case INVALIDATE_LINE:
      op = new Memory::InvalidateLine(address, metadata, returnAddress); break;
    case FLUSH_ALL_LINES:
      op = new Memory::FlushAllLines(address, metadata, returnAddress); break;
    case INVALIDATE_ALL_LINES:
      op = new Memory::InvalidateAllLines(address, metadata, returnAddress); break;
    case STORE_W:
      op = new Memory::StoreWord(address, metadata, returnAddress); break;
    case STORE_CONDITIONAL:
      op = new Memory::StoreConditional(address, metadata, returnAddress); break;
    case STORE_HW:
      op = new Memory::StoreHalfword(address, metadata, returnAddress); break;
    case STORE_B:
      op = new Memory::StoreByte(address, metadata, returnAddress); break;
    case STORE_LINE:
      op = new Memory::StoreLine(address, metadata, returnAddress); break;
    case MEMSET_LINE:
      op = new Memory::MemsetLine(address, metadata, returnAddress); break;
    case PUSH_LINE:
      op = new Memory::PushLine(address, metadata, returnAddress); break;
    case LOAD_AND_ADD:
      op = new Memory::LoadAndAdd(address, metadata, returnAddress); break;
    case LOAD_AND_OR:
      op = new Memory::LoadAndOr(address, metadata, returnAddress); break;
    case LOAD_AND_AND:
      op = new Memory::LoadAndAnd(address, metadata, returnAddress); break;
    case LOAD_AND_XOR:
      op = new Memory::LoadAndXor(address, metadata, returnAddress); break;
    case EXCHANGE:
      op = new Memory::Exchange(address, metadata, returnAddress); break;
    case UPDATE_DIRECTORY_ENTRY:
      op = new Memory::UpdateDirectoryEntry(address, metadata, returnAddress); break;
    case UPDATE_DIRECTORY_MASK:
      op = new Memory::UpdateDirectoryMask(address, metadata, returnAddress); break;

    default:
      throw InvalidOptionException("memory request opcode", (int)metadata.opcode);
      op = NULL;
      break;

  }

  op->assignToMemory(memory, level);
  return op;

}



#endif /* SRC_TILE_MEMORY_OPERATIONS_MEMORYOPERATIONDECODE_H_ */
