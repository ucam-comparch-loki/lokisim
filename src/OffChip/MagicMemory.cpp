/*
 * MagicMemory.cpp
 *
 *  Created on: 15 Oct 2015
 *      Author: db434
 */

#include "MagicMemory.h"
#include "MainMemory.h"
#include "../Chip.h"
#include "../Datatype/Instruction.h"
#include "../Exceptions/InvalidOptionException.h"
#include "../Network/NetworkTypes.h"
#include "../Utility/Assert.h"
#include "../Utility/Parameters.h"

MagicMemory::MagicMemory(const sc_module_name& name, MainMemory& mainMemory) :
    LokiComponent(name),
    mainMemory(mainMemory) {
  // Nothing
}

void MagicMemory::operate(MemoryOpcode opcode,
                          MemoryAddr address,
                          ChannelID returnChannel,
                          Word data) {
  LOKI_LOG(1) << this->name() << " performing " << memoryOpName(opcode) << " "
      << LOKI_HEX(address) << " for " << returnChannel << endl;

  switch (opcode) {
    case LOAD_W: {
      Word result = mainMemory.readWord(address, MEMORY_SCRATCHPAD, true);
      NetworkData flit(result, returnChannel, true);
      chip().networkSendDataInternal(flit);
      break;
    }

    case LOAD_HW: {
      Word result = mainMemory.readHalfword(address, MEMORY_SCRATCHPAD, true);
      NetworkData flit(result, returnChannel, true);
      chip().networkSendDataInternal(flit);
      break;
    }

    case LOAD_B: {
      Word result = mainMemory.readByte(address, MEMORY_SCRATCHPAD, true);
      NetworkData flit(result, returnChannel, true);
      chip().networkSendDataInternal(flit);
      break;
    }

    case STORE_W:
      mainMemory.writeWord(address, data.toUInt(), MEMORY_SCRATCHPAD, true);
      break;

    case STORE_HW:
      mainMemory.writeHalfword(address, data.toUInt(), MEMORY_SCRATCHPAD, true);
      break;

    case STORE_B:
      mainMemory.writeByte(address, data.toUInt(), MEMORY_SCRATCHPAD, true);
      break;

    case IPK_READ: {
      MemoryAddr cursor = address;

      while (true) {
        Instruction result = static_cast<Instruction>(mainMemory.readWord(cursor, MEMORY_SCRATCHPAD, true));
        cursor += BYTES_PER_WORD;

        // Stop fetching instructions at the end of the packet or the end
        // of the cache line.
        bool endOfPacket = result.endOfPacket() || ((cursor & 0x1F) == 0);

        NetworkData flit(result, returnChannel, endOfPacket);
        chip().networkSendDataInternal(flit);

        if (endOfPacket)
          break;
      }
      break;
    }

    case MEMSET_LINE: {
      MemoryAddr addr = address & ~0x1f;
      MemoryAddr end = addr + 0x20;
      for (; addr < end; addr += BYTES_PER_WORD)
        mainMemory.writeWord(addr, data.toUInt(), MEMORY_SCRATCHPAD, true);
      break;
    }

    case LOAD_AND_ADD: {
      Word result = mainMemory.readWord(address, MEMORY_SCRATCHPAD, true);
      NetworkData flit(result, returnChannel, true);
      chip().networkSendDataInternal(flit);
      mainMemory.writeWord(address, result.toUInt() + data.toUInt(), MEMORY_SCRATCHPAD, true);
      break;
    }

    case LOAD_AND_OR: {
      Word result = mainMemory.readWord(address, MEMORY_SCRATCHPAD, true);
      NetworkData flit(result, returnChannel, true);
      chip().networkSendDataInternal(flit);
      mainMemory.writeWord(address, result.toUInt() | data.toUInt(), MEMORY_SCRATCHPAD, true);
      break;
    }

    case LOAD_AND_AND: {
      Word result = mainMemory.readWord(address, MEMORY_SCRATCHPAD, true);
      NetworkData flit(result, returnChannel, true);
      chip().networkSendDataInternal(flit);
      mainMemory.writeWord(address, result.toUInt() & data.toUInt(), MEMORY_SCRATCHPAD, true);
      break;
    }

    case LOAD_AND_XOR: {
      Word result = mainMemory.readWord(address, MEMORY_SCRATCHPAD, true);
      NetworkData flit(result, returnChannel, true);
      chip().networkSendDataInternal(flit);
      mainMemory.writeWord(address, result.toUInt() ^ data.toUInt(), MEMORY_SCRATCHPAD, true);
      break;
    }

    case EXCHANGE: {
      Word result = mainMemory.readWord(address, MEMORY_SCRATCHPAD, true);
      NetworkData flit(result, returnChannel, true);
      chip().networkSendDataInternal(flit);
      mainMemory.writeWord(address, data.toUInt(), MEMORY_SCRATCHPAD, true);
      break;
    }

    case VALIDATE_LINE:
    case PREFETCH_LINE:
    case FLUSH_LINE:
    case INVALIDATE_LINE:
    case FLUSH_ALL_LINES:
    case INVALIDATE_ALL_LINES:
    case PUSH_LINE:
    case UPDATE_DIRECTORY_ENTRY:
    case UPDATE_DIRECTORY_MASK:
      LOKI_LOG(2) << this->name() << ": " << memoryOpName(opcode) << " has no effect" << endl;
      break;

    default:
      loki_assert_with_message(false, "Magic memory doesn't yet support %s", memoryOpName(opcode).c_str());
      break;
  }
}

Chip& MagicMemory::chip() const {
  return *static_cast<Chip*>(this->get_parent_object());
}
