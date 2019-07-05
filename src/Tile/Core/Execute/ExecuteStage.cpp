/*
 * ExecuteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "ExecuteStage.h"

#include "../../../Utility/Assert.h"
#include "../../../Utility/Instrumentation.h"
#include "../../../Utility/Instrumentation/Registers.h"
#include "../../../Exceptions/InvalidOptionException.h"
#include "../../../Exceptions/UnsupportedFeatureException.h"
#include "../../../Utility/Logging.h"
#include "../Core.h"

bool ExecuteStage::readPredicate() const {return core().readPredReg();}
int32_t ExecuteStage::readReg(RegisterIndex reg) const {return core().readReg(1, reg);}
int32_t ExecuteStage::readWord(MemoryAddr addr) const {return core().readWord(addr).toInt();}
int32_t ExecuteStage::readByte(MemoryAddr addr) const {return core().readByte(addr).toInt();}

void ExecuteStage::writePredicate(bool val) const {core().writePredReg(val);}
void ExecuteStage::writeReg(RegisterIndex reg, Word data) const {core().writeReg(reg, data.toInt());}
void ExecuteStage::writeWord(MemoryAddr addr, Word data) const {core().writeWord(addr, data);}
void ExecuteStage::writeByte(MemoryAddr addr, Word data) const {core().writeByte(addr, data);}

void ExecuteStage::execute() {
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
    LOKI_LOG(3) << this->name() << " stalled." << endl;
}

void ExecuteStage::newInput(DecodedInst& operation) {
  // See if the instruction should execute.
  bool willExecute = checkPredicate(operation);

  if (willExecute) {
    bool success = true;

    // Only collect operands on the first cycle of multi-cycle operations.
    if (alu.busy()) {
      LOKI_LOG(2) << this->name() << ": continuing " << operation.name()
          << " on " << operation.operand1() << " and " << operation.operand2() << endl;
    }
    else if (isStalled()) {
      LOKI_LOG(2) << this->name() << ": expanding " << operation << " to multiple operations" << endl;
    }
    else {
      // Forward data from the previous instruction if necessary.
      if (operation.operand1Source() == DecodedInst::BYPASS && previousInstExecuted) {
        operation.operand1(forwardedResult);
        operation.operand1Source(DecodedInst::IMMEDIATE); // So we don't forward again.
        LOKI_LOG(2) << this->name() << " forwarding contents of register "
            << (int)operation.sourceReg1() << ": " << forwardedResult << endl;
        if (ENERGY_TRACE)
          Instrumentation::Registers::forward(1);
      }
      if (operation.operand2Source() == DecodedInst::BYPASS && previousInstExecuted) {
        operation.operand2(forwardedResult);
        operation.operand2Source(DecodedInst::IMMEDIATE); // So we don't forward again.
        LOKI_LOG(2) << this->name() << " forwarding contents of register "
            << (int)operation.sourceReg2() << ": " << forwardedResult << endl;
        if (ENERGY_TRACE)
          Instrumentation::Registers::forward(2);
      }

      LOKI_LOG(1) << this->name() << ": executing " << LOKI_HEX(operation.location()) << " " << operation.name()
          << " on " << operation.operand1() << " and " << operation.operand2() << endl;
    }

    // Special cases for any instructions which don't use the ALU.
    switch (operation.opcode()) {
      case ISA::OP_SETCHMAP:
      case ISA::OP_SETCHMAPI:
        setChannelMap(operation);
        success = !blocked;
        break;

      case ISA::OP_GETCHMAP:
        operation.result(core().channelMapTable.read(operation.operand1()));
        break;

      case ISA::OP_GETCHMAPI:
        operation.result(core().channelMapTable.read(operation.immediate()));
        break;

      case ISA::OP_CREGRDI:
        operation.result(core().cregs.read(operation.immediate()));
        break;

      case ISA::OP_CREGWRI:
        core().cregs.write(operation.immediate(), operation.operand1());
        break;

      case ISA::OP_SCRATCHRD:
        // Send only lowest 8 bits of address - don't need mask in hardware.
        operation.result(scratchpad.read(operation.operand1() & 0xFF));
        break;

      case ISA::OP_SCRATCHRDI:
        // Send only lowest 8 bits of address - don't need mask in hardware.
        operation.result(scratchpad.read(operation.operand2() & 0xFF));
        break;

      case ISA::OP_SCRATCHWR:
      case ISA::OP_SCRATCHWRI:
        // Send only lowest 8 bits of address - don't need mask in hardware.
        scratchpad.write(operation.operand2() & 0xFF, operation.operand1());
        break;

      case ISA::OP_IRDR:
        operation.result(operation.operand1());
        break;

      case ISA::OP_IWTR:
        operation.result(operation.operand1());
        break;

      case ISA::OP_LLI:
        operation.result(operation.operand2());
        break;

      case ISA::OP_LUI:
        operation.result(operation.operand1() | (operation.operand2() << 16));
        break;

      case ISA::OP_STW:
      case ISA::OP_STHW:
      case ISA::OP_STB:
      case ISA::OP_STC:
      case ISA::OP_LDADD:
      case ISA::OP_LDAND:
      case ISA::OP_LDOR:
      case ISA::OP_LDXOR:
      case ISA::OP_EXCHANGE:
        if (!continuingStore)
          memoryStorePhase1(operation);
        else
          memoryStorePhase2(operation);
        break;

      case ISA::OP_SENDCONFIG:
        operation.result(operation.operand1());
        break;

      case ISA::OP_SYSCALL:
        // TODO: remove from ALU.
        alu.systemCall(operation);
        break;

      default:
        if (ISA::isALUOperation(operation.opcode())) {
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

    // HACK: if we removed a credit in DecodeStage::waitOnCredits, and now
    // discover that we aren't going to execute the instruction, add the credit
    // back again.
    if (currentInst.sendsOnNetwork())
      core().channelMapTable.addCredit(currentInst.channelMapEntry(), 1);
  }

  // Only instrument operations which executed in this pipeline stage.
  // PAYLOAD means this is the second half of a store operation - we don't
  // want to instrument it twice.
  if (operation.isExecuteStageOperation() &&
      operation.memoryOp() != PAYLOAD &&
      operation.memoryOp() != PAYLOAD_EOP &&
      !blocked) {
    Instrumentation::executed(core(), operation, willExecute);

    // Note: there is a similar call from the decode stage for instructions
    // which complete their execution there.
    core().cregs.instructionExecuted();
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
    if (core().isMemory(currentInst.networkDestination().component))
      adjustNetworkAddress(currentInst);

    if (MAGIC_MEMORY && !currentInst.forRemoteExecution() &&
        core().isMemory(currentInst.networkDestination().component)) {
      core().magicMemoryAccess(currentInst);
    }
    else {
      // Send the data to the output buffer - it will arrive immediately so that
      // network resources can be requested the cycle before they are used.
      loki_assert(!oData.valid());
      oData.write(currentInst.toNetworkData(id().tile));
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
  core().channelMapTable.write(entry, value);
}

void ExecuteStage::memoryStorePhase1(DecodedInst& operation) {
  // Result = memory address to access.
  operation.result(operation.operand2() + operation.immediate());
  operation.endOfNetworkPacket(false);
  continuingStore = true;

  if (MAGIC_MEMORY)
    next_trigger(clock.posedge_event() & nextStageUnblockedEvent());
  else
    next_trigger(clock.posedge_event() & nextStageUnblockedEvent() & oData.ack_event());
}

void ExecuteStage::memoryStorePhase2(DecodedInst& operation) {
  // Result = data to store.
  operation.result(operation.operand1());
  operation.endOfNetworkPacket(true);
  operation.memoryOp(PAYLOAD_EOP);
  continuingStore = false;
}

void ExecuteStage::adjustNetworkAddress(DecodedInst& inst) {
  loki_assert_with_message(core().isMemory(inst.networkDestination().component),
      "Destination = %s", inst.networkDestination().getString(Encoding::hardwareChannelID).c_str());

  // Not sure why we have to get a fresh copy of the channel map table entry.
  ChannelMapEntry& channelMapEntry = core().channelMapTable[inst.channelMapEntry()];
  ChannelMapEntry::MemoryChannel channel = channelMapEntry.memoryView();
//  ChannelMapEntry::MemoryChannel channel =
//      ChannelMapEntry::memoryView(inst.cmtEntry());

  // Adjust destination channel based on memory configuration if necessary
  ChannelID destination = bankSelector.getMapping(inst.memoryOp(),
      (int)inst.result(), channel);
  uint32_t increment = destination.component.position;

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
    case ISA::FN_ADDU: {
      uint64_t val1 = (uint64_t)((uint32_t)inst.operand1());
      uint64_t val2 = (uint64_t)((uint32_t)inst.operand2());
      uint64_t result64 = val1 + val2;
      newPredicate = (result64 >> 32) != 0;
      break;
    }

    // The 68k and x86 set the borrow bit if a - b < 0 for subtractions.
    // The 6502 and PowerPC treat it as a carry bit.
    // http://en.wikipedia.org/wiki/Carry_flag#Carry_flag_vs._Borrow_flag
    case ISA::FN_SUBU: {
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

ExecuteStage::ExecuteStage(const sc_module_name& name,
                           const scratchpad_parameters_t& spadParams) :
    PipelineStage(name),
    oReady("oReady"),
    oData("oData"),
    iReady("iReady"),
    alu("alu"),
    scratchpad("scratchpad", spadParams),
    bankSelector(id()) {

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
