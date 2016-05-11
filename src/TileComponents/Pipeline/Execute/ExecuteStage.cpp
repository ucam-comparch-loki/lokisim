/*
 * ExecuteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "ExecuteStage.h"
#include "../../Core.h"
#include "../../../Utility/Assert.h"
#include "../../../Utility/Instrumentation.h"
#include "../../../Utility/Instrumentation/Registers.h"
#include "../../../Exceptions/InvalidOptionException.h"
#include "../../../Exceptions/UnsupportedFeatureException.h"

bool ExecuteStage::readPredicate() const {return core()->readPredReg();}
int32_t ExecuteStage::readReg(RegisterIndex reg) const {return core()->readReg(1, reg);}
int32_t ExecuteStage::readWord(MemoryAddr addr) const {return core()->readWord(addr).toInt();}
int32_t ExecuteStage::readByte(MemoryAddr addr) const {return core()->readByte(addr).toInt();}

void ExecuteStage::writePredicate(bool val) const {core()->writePredReg(val);}
void ExecuteStage::writeReg(RegisterIndex reg, Word data) const {core()->writeReg(reg, data.toInt());}
void ExecuteStage::writeWord(MemoryAddr addr, Word data) const {core()->writeWord(addr, data);}
void ExecuteStage::writeByte(MemoryAddr addr, Word data) const {core()->writeByte(addr, data);}

void ExecuteStage::execute() {

  LOKI_LOG << this->name() << " received Instruction: " << currentInst << endl;

  // Wait until it is clear to produce network data.
  if (currentInst.sendsOnNetwork() && !iReady.read()) {
    blocked = true;
    next_trigger(iReady.posedge_event());
    return;
  }
  else if (currentInst.sendsOnNetwork() && oData.valid()) {
    blocked = true;
    next_trigger(oData.ack_event());
    return;
  }
  blocked = false;

  // If there is already a result, don't do anything
  if (currentInst.hasResult() && !continuingStore) {
    previousInstExecuted = true;
    if (currentInst.setsPredicate())
      updatePredicate(currentInst);
    sendOutput();
  }
  else
    newInput(currentInst);

  forwardedResult = currentInst.result();

  if (!blocked && !continuingStore) {
    instructionCompleted();
  }

}

void ExecuteStage::updateReady() {
  bool ready = !isStalled();

  // Write our current stall status.
  if (ready != oReady.read())
    oReady.write(ready);

  if (isStalled() && oReady.read())
    LOKI_LOG << this->name() << " stalled." << endl;
}

void ExecuteStage::newInput(DecodedInst& operation) {
  // See if the instruction should execute.
  bool willExecute = checkPredicate(operation);

  if (willExecute) {
    bool success = true;

    // Only collect operands on the first cycle of multi-cycle operations.
    if (alu.busy()) {
      LOKI_LOG << this->name() << ": continuing " << operation.name()
          << " on " << operation.operand1() << " and " << operation.operand2() << endl;
    }
    else {
      // Forward data from the previous instruction if necessary.
      if (operation.operand1Source() == DecodedInst::BYPASS && previousInstExecuted) {
        operation.operand1(forwardedResult);
        operation.operand1Source(DecodedInst::IMMEDIATE); // So we don't forward again.
        LOKI_LOG << this->name() << " forwarding contents of register "
            << (int)operation.sourceReg1() << ": " << forwardedResult << endl;
        if (ENERGY_TRACE)
          Instrumentation::Registers::forward(1);
      }
      if (operation.operand2Source() == DecodedInst::BYPASS && previousInstExecuted) {
        operation.operand2(forwardedResult);
        operation.operand2Source(DecodedInst::IMMEDIATE); // So we don't forward again.
        LOKI_LOG << this->name() << " forwarding contents of register "
            << (int)operation.sourceReg2() << ": " << forwardedResult << endl;
        if (ENERGY_TRACE)
          Instrumentation::Registers::forward(2);
      }

      LOKI_LOG << this->name() << ": executing " << operation.name()
          << " on " << operation.operand1() << " and " << operation.operand2() << endl;
    }

    // Special cases for any instructions which don't use the ALU.
    switch (operation.opcode()) {
      case InstructionMap::OP_SETCHMAP:
      case InstructionMap::OP_SETCHMAPI:
        setChannelMap(operation);
        success = !blocked;
        break;

      case InstructionMap::OP_GETCHMAP:
        operation.result(core()->channelMapTable.read(operation.operand1()));
        break;

      case InstructionMap::OP_GETCHMAPI:
        operation.result(core()->channelMapTable.read(operation.immediate()));
        break;

      case InstructionMap::OP_CREGRDI:
        operation.result(core()->cregs.read(operation.immediate()));
        break;

      case InstructionMap::OP_CREGWRI:
        core()->cregs.write(operation.immediate(), operation.operand1());
        break;

      case InstructionMap::OP_SCRATCHRD:
        // Send only lowest 8 bits of address - don't need mask in hardware.
        operation.result(scratchpad.read(operation.operand1() & 0xFF));
        break;

      case InstructionMap::OP_SCRATCHRDI:
        // Send only lowest 8 bits of address - don't need mask in hardware.
        operation.result(scratchpad.read(operation.operand2() & 0xFF));
        break;

      case InstructionMap::OP_SCRATCHWR:
      case InstructionMap::OP_SCRATCHWRI:
        // Send only lowest 8 bits of address - don't need mask in hardware.
        scratchpad.write(operation.operand2() & 0xFF, operation.operand1());
        break;

      case InstructionMap::OP_IRDR:
        operation.result(operation.operand1());
        break;

      case InstructionMap::OP_IWTR:
        operation.result(operation.operand1());
        break;

      case InstructionMap::OP_LLI:
        operation.result(operation.operand2());
        break;

      case InstructionMap::OP_LUI:
        operation.result(operation.operand1() | (operation.operand2() << 16));
        break;

      case InstructionMap::OP_STW:
      case InstructionMap::OP_STHW:
      case InstructionMap::OP_STB:
      case InstructionMap::OP_STC:
      case InstructionMap::OP_LDADD:
      case InstructionMap::OP_LDAND:
      case InstructionMap::OP_LDOR:
      case InstructionMap::OP_LDXOR:
      case InstructionMap::OP_EXCHANGE:
        if (!continuingStore)
          memoryStorePhase1(operation);
        else
          memoryStorePhase2(operation);
        break;

      case InstructionMap::OP_SENDCONFIG:
        operation.result(operation.operand1());
        break;

      case InstructionMap::OP_SYSCALL:
        // TODO: remove from ALU.
        alu.systemCall(operation);
        break;

      default:
        if (InstructionMap::isALUOperation(operation.opcode())) {
          alu.execute(operation);
          blocked = alu.busy();
          success = !blocked;

          if (blocked)
            next_trigger(clock.posedge_event());
        }

        break;
    } // end switch

    if (operation.setsPredicate())
      updatePredicate(operation);

    if (success)
      sendOutput();

  } // end if (will execute)
  else {
    // If the instruction will not be executed, invalidate it so we don't
    // try to forward data from it.
    currentInst.preventForwarding();
  }

  // Only instrument operations which executed in this pipeline stage.
  // PAYLOAD means this is the second half of a store operation - we don't
  // want to instrument it twice.
  if (operation.isExecuteStageOperation() &&
      operation.memoryOp() != PAYLOAD &&
      operation.memoryOp() != PAYLOAD_EOP &&
      !blocked) {
    Instrumentation::executed(id, operation, willExecute);

    // Note: there is a similar call from the decode stage for instructions
    // which complete their execution there.
    core()->cregs.instructionExecuted();
  }

  previousInstExecuted = willExecute;
}

void ExecuteStage::sendOutput() {
  if (currentInst.sendsOnNetwork()) {
    // Memory operations may be sent to different memory banks depending on the
    // address accessed.
    // In practice, this would be performed by a separate, small functional
    // unit in parallel with the main ALU, so that there is time left to request
    // a path to memory.
    if (currentInst.networkDestination().isMemory())
      adjustNetworkAddress(currentInst);

    if (MAGIC_MEMORY && currentInst.networkDestination().isMemory()) {
      ChannelMapEntry& channelMapEntry = core()->channelMapTable[currentInst.channelMapEntry()];
      ChannelID returnChannel(id.tile.x, id.tile.y, channelMapEntry.getChannel(), channelMapEntry.getReturnChannel());

      switch (currentInst.memoryOp()) {
        case LOAD_W:
        case LOAD_HW:
        case LOAD_B:
          core()->magicMemoryAccess(currentInst.memoryOp(), currentInst.result(),
                                    returnChannel);
          break;

        case STORE_W:
        case STORE_HW:
        case STORE_B:
          core()->magicMemoryAccess(currentInst.memoryOp(), currentInst.result(),
                                    returnChannel, currentInst.operand1());
          break;

        case PAYLOAD:
        case PAYLOAD_EOP:
          // Don't send payloads as separate flits - they're sent with the
          // header when accessing magic memory.
          break;

        default:
          throw InvalidOptionException("magic memory operation", currentInst.memoryOp());
          break;
      }
    }
    else {
      // Send the data to the output buffer - it will arrive immediately so that
      // network resources can be requested the cycle before they are used.
      loki_assert(!oData.valid());
      oData.write(currentInst.toNetworkData(id.tile));
    }
  }

  // Send the data to the register file - it will arrive at the beginning
  // of the next clock cycle.
  outputInstruction(currentInst);
}

void ExecuteStage::setChannelMap(DecodedInst& inst) {
  MapIndex entry = inst.operand2();
  uint32_t value = inst.operand1();

  // Write to the channel map table.
  core()->channelMapTable.write(entry, value);
}

void ExecuteStage::memoryStorePhase1(DecodedInst& operation) {
  // Result = memory address to access.
  operation.result(operation.operand2() + operation.immediate());
  operation.endOfNetworkPacket(false);
  continuingStore = true;

  if (MAGIC_MEMORY)
    next_trigger(clock.posedge_event() & canSendEvent());
  else
    next_trigger(clock.posedge_event() & canSendEvent() & oData.ack_event());
}

void ExecuteStage::memoryStorePhase2(DecodedInst& operation) {
  // Result = data to store.
  operation.result(operation.operand1());
  operation.endOfNetworkPacket(true);
  operation.memoryOp(PAYLOAD_EOP);
  continuingStore = false;
}

void ExecuteStage::adjustNetworkAddress(DecodedInst& inst) const {
  loki_assert_with_message(inst.networkDestination().isMemory(),
      "Destination = %s", inst.networkDestination().getString().c_str());

  bool addressFlit;

  switch (inst.memoryOp()) {
    case PAYLOAD:
    case PAYLOAD_EOP:
    case UPDATE_DIRECTORY_ENTRY:
    case UPDATE_DIRECTORY_MASK:
      addressFlit = false;
      break;
    default:
      addressFlit = true;
      break;
  }

  // We want to access lots of information from the channel map table, so get
  // the entire entry.
  ChannelMapEntry& channelMapEntry = core()->channelMapTable[inst.channelMapEntry()];

  // Adjust destination channel based on memory configuration if necessary
  uint32_t increment = 0;

  if (channelMapEntry.getDestination().isMemory() &&
      channelMapEntry.getMemoryGroupSize() > 1) {
    if (addressFlit) {
      increment = channelMapEntry.computeAddressIncrement((uint32_t)inst.result());

      // Store the adjustment which must be made, so that any subsequent flits
      // can also access the same memory bank.
      channelMapEntry.setAddressIncrement(increment);
    }
    else {
      increment = channelMapEntry.getAddressIncrement();
    }
  }

  ChannelMapEntry::MemoryChannel channel = channelMapEntry.memoryView();
  channel.bank += increment;
  inst.cmtEntry(channel.flatten());
}

bool ExecuteStage::isStalled() const {
  // When we have multi-cycle operations (e.g. multiplies), we will need to use
  // this.
  return !iReady.read() || blocked || continuingStore;
}

bool ExecuteStage::checkPredicate(DecodedInst& inst) {
  bool pred = readPredicate();
  short predBits = inst.predicate();

  bool result = (predBits == Instruction::ALWAYS) ||
                (predBits == Instruction::END_OF_PACKET) ||
                (predBits == Instruction::P     &&  pred) ||
                (predBits == Instruction::NOT_P && !pred);

  return result;
}

void ExecuteStage::updatePredicate(const DecodedInst& inst) {
  loki_assert(inst.setsPredicate());

  bool newPredicate;

  switch (inst.function()) {
    // For additions and subtractions, the predicate represents the carry
    // and borrow bits, respectively.
    case InstructionMap::FN_ADDU: {
      uint64_t val1 = (uint64_t)((uint32_t)inst.operand1());
      uint64_t val2 = (uint64_t)((uint32_t)inst.operand2());
      uint64_t result64 = val1 + val2;
      newPredicate = (result64 >> 32) != 0;
      break;
    }

    // The 68k and x86 set the borrow bit if a - b < 0 for subtractions.
    // The 6502 and PowerPC treat it as a carry bit.
    // http://en.wikipedia.org/wiki/Carry_flag#Carry_flag_vs._Borrow_flag
    case InstructionMap::FN_SUBU: {
      uint64_t val1 = (uint64_t)((uint32_t)inst.operand1());
      uint64_t val2 = (uint64_t)((uint32_t)inst.operand2());
      newPredicate = val1 < val2;
      break;
    }

    // Otherwise, it holds the least significant bit of the result.
    // Potential alternative: newPredicate = (result != 0)
    default:
      newPredicate = inst.result() & 1;
      break;
  }

  writePredicate(newPredicate);
}

const sc_event& ExecuteStage::executedEvent() const {
  return instructionCompletedEvent;
}

void ExecuteStage::reportStalls(ostream& os) {
  if (blocked) {
    os << this->name() << " blocked while executing " << currentInst << endl;
  }
}

ExecuteStage::ExecuteStage(const sc_module_name& name, const ComponentID& ID) :
    PipelineStage(name, ID),
    alu("alu", ID),
    scratchpad("scratchpad", ID) {

  forwardedResult = 0;
  previousInstExecuted = false;
  blocked = false;
  continuingStore = false;

  SC_METHOD(execute);
  sensitive << newInstructionEvent;
  dont_initialize();

  SC_METHOD(updateReady);
  sensitive << instructionCompletedEvent;
  // do initialise

}
