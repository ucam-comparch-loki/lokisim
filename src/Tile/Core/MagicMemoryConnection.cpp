/*
 * MagicMemoryConnection.cpp
 *
 *  Created on: 30 Jan 2017
 *      Author: db434
 */

#include "MagicMemoryConnection.h"
#include "../../Datatype/MemoryOperations/MemoryOperationDecode.h"
#include "../../Utility/Assert.h"
#include "../../Utility/ISA.h"
#include "Core.h"

MagicMemoryConnection::MagicMemoryConnection(sc_module_name name, ComponentID id) :
    LokiComponent(name, id) {

  currentOpcode = PAYLOAD_EOP;
  currentAddress = -1;
  payloadsRemaining = 0;

}

MagicMemoryConnection::~MagicMemoryConnection() {
  // Do nothing
}

unsigned int numPayloadFlits(MemoryOpcode op) {
  // TODO: would prefer to do this by decoding the operation and querying its
  // state, but that currently requires a reference to a MemoryBase, and we
  // don't have one of those here.

  // The memory address always goes in the head flit. Any remaining data
  // follows in payload flits.
  switch (op) {
    // End-of-packet flits
    case LOAD_W:
    case LOAD_LINKED:
    case LOAD_HW:
    case LOAD_B:
    case FETCH_LINE:
    case IPK_READ:
    case VALIDATE_LINE:
    case PREFETCH_LINE:
    case FLUSH_LINE:
    case INVALIDATE_LINE:
    case FLUSH_ALL_LINES:
    case INVALIDATE_ALL_LINES:
    case NONE:
    case PAYLOAD:
    case PAYLOAD_EOP:
      return 0;
      break;

    // Start/mid-packet flits
    case STORE_W:
    case STORE_CONDITIONAL:
    case STORE_HW:
    case STORE_B:
    case MEMSET_LINE:
    case LOAD_AND_ADD:
    case LOAD_AND_OR:
    case LOAD_AND_AND:
    case LOAD_AND_XOR:
    case EXCHANGE:
    case UPDATE_DIRECTORY_ENTRY:
    case UPDATE_DIRECTORY_MASK:
      return 1;
      break;

    case STORE_LINE:
    case PUSH_LINE:
      return 8;
      break;

    default:
      assert(false);
      return 0;
      break;
  }
}

void MagicMemoryConnection::operate(const DecodedInst& instruction) {
  loki_assert(instruction.networkDestination().isMemory());

  ChannelMapEntry& channelMapEntry = parent()->channelMapTable[instruction.channelMapEntry()];
  ChannelID returnChannel(id.tile.x, id.tile.y, channelMapEntry.getChannel(), channelMapEntry.getReturnChannel());

  loki_assert_with_message(!channelMapEntry.memoryView().scratchpadL1,
      "Magic memory only supports one address space.", 0);

  MemoryOpcode memoryOp = instruction.memoryOp();

  // If this is a sendconfig instruction, then all values will arrive in
  // separate instructions. If any other instruction wants to access memory,
  // it includes all necessary data in a single instruction.
  if (instruction.opcode() == ISA::OP_SENDCONFIG) {

    // Operation continuing.
    if (memoryOp == PAYLOAD || memoryOp == PAYLOAD_EOP) {
      loki_assert(currentOpcode != PAYLOAD_EOP);
      loki_assert(currentOpcode != PAYLOAD);
      loki_assert(payloadsRemaining > 0);

      parent()->magicMemoryAccess(currentOpcode, currentAddress,
          returnChannel, instruction.result());

      // All multi-payload operations act on words (rather than bytes, etc.).
      currentAddress += 4;
      payloadsRemaining--;
    }
    // Operation starting.
    else {
      currentOpcode = memoryOp;
      currentAddress = instruction.result();
      payloadsRemaining = numPayloadFlits(memoryOp);

      // If we have a multi-payload operation, convert it into a sequence of
      // single-payload operations.
      if (memoryOp == STORE_LINE || memoryOp == PUSH_LINE)
        currentOpcode = STORE_W;
      else if (payloadsRemaining > 1)
        loki_assert_with_message(false, "Unhandled complex operation: %s", memoryOpName(memoryOp).c_str());
      else if (payloadsRemaining == 0)
        parent()->magicMemoryAccess(currentOpcode, currentAddress, returnChannel);
    }

    // Operation finished.
    if (payloadsRemaining == 0) {
      currentOpcode = PAYLOAD_EOP;
      currentAddress = -1;
    }

  }
  // Normal memory operations.
  else {
    // Payloads are handled with the head flit, so do not need to be handled
    // again.
    if (memoryOp != PAYLOAD && memoryOp != PAYLOAD_EOP) {
      int payloads = numPayloadFlits(memoryOp);
      switch (payloads) {
        case 0:
          parent()->magicMemoryAccess(memoryOp, instruction.result(), returnChannel);
          break;
        case 1:
          parent()->magicMemoryAccess(memoryOp, instruction.result(),
              returnChannel, instruction.operand1());
          break;
        default:
          loki_assert_with_message(false, "Magic memory doesn't support multi-payload operations.", 0);
          break;
      }
    }
  }
}

Core* MagicMemoryConnection::parent() const {
  return static_cast<Core*>(this->get_parent_object());
}
