/*
 * MagicMemory.cpp
 *
 *  Created on: 15 Oct 2015
 *      Author: db434
 */

#include "MagicMemory.h"
#include "../../Chip.h"
#include "../../Datatype/Instruction.h"
#include "../../Exceptions/InvalidOptionException.h"
#include "../../Network/NetworkTypedefs.h"
#include "../../Utility/Parameters.h"
#include "SimplifiedOnChipScratchpad.h"

MagicMemory::MagicMemory(const sc_module_name& name, SimplifiedOnChipScratchpad& mainMemory) :
    Component(name),
    mainMemory(mainMemory) {
  // Nothing
}

void MagicMemory::operate(MemoryOpcode opcode,
                          MemoryAddr address,
                          ChannelID returnChannel,
                          Word data) {
  LOKI_LOG << this->name() << " performing " << memoryOpName(opcode) << " "
      << LOKI_HEX(address) << " for " << returnChannel << endl;

  switch (opcode) {
    case LOAD_W: {
      Word result = mainMemory.readWord(address);
      NetworkData flit(result, returnChannel, true);
      chip().networkSendDataInternal(flit);
      break;
    }

    case LOAD_HW: {
      Word result = mainMemory.readHalfword(address);
      NetworkData flit(result, returnChannel, true);
      chip().networkSendDataInternal(flit);
      break;
    }

    case LOAD_B: {
      Word result = mainMemory.readByte(address);
      NetworkData flit(result, returnChannel, true);
      chip().networkSendDataInternal(flit);
      break;
    }

    case STORE_W:
      mainMemory.writeWord(address, data);
      break;

    case STORE_HW:
      mainMemory.writeHalfword(address, data);
      break;

    case STORE_B:
      mainMemory.writeByte(address, data);
      break;

    case IPK_READ: {
      MemoryAddr cursor = address;

      while (true) {
        Instruction result = static_cast<Instruction>(mainMemory.readWord(cursor));
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

    default:
      throw InvalidOptionException("magic memory operation", opcode);
      break;
  }
}

Chip& MagicMemory::chip() const {
  return *static_cast<Chip*>(this->get_parent_object());
}
