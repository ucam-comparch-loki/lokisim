/*
 * ExecuteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "ExecuteStage.h"
#include "../../Core.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../../../Utility/Instrumentation.h"
#include "../../../Utility/Instrumentation/Registers.h"
#include "../../../Utility/Trace/LBTTrace.h"
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
//  if (alu.busy()) {
//    if (DEBUG) cout << this->name() << ": continuing " << currentInst.name()
//        << " on " << currentInst.operand1() << " and " << currentInst.operand2() << endl;
//    alu.execute(currentInst);
//    blocked = alu.busy();
//  }

  if (currentInst.sendsOnNetwork() && !iReady.read()) {
    next_trigger(iReady.posedge_event());
    return;
  }

  if (DEBUG)
    cout << this->name() << " received Instruction: " << currentInst << endl;

  // If there is already a result, don't do anything
  if (currentInst.hasResult()) {
    if (currentInst.setsPredicate())
      updatePredicate(currentInst);
    sendOutput();
  }
  else
    newInput(currentInst);

  forwardedResult = currentInst.result();

  if (!blocked) {
    instructionCompleted();
  }
}

void ExecuteStage::updateReady() {
  bool ready = !isStalled();

  // Write our current stall status.
  if (ready != oReady.read())
    oReady.write(ready);

  if (DEBUG && isStalled() && oReady.read())
    cout << this->name() << " stalled." << endl;
}

void ExecuteStage::newInput(DecodedInst& operation) {
  // See if the instruction should execute.
  bool willExecute = checkPredicate(operation);

  if (Arguments::lbtTrace())
	LBTTrace::setInstructionExecuteFlag(operation.isid(), willExecute);

  if (willExecute) {
    bool success = true;

    // Only collect operands on the first cycle of multi-cycle operations.
    if (alu.busy()) {
      if (DEBUG) cout << this->name() << ": continuing " << operation.name()
          << " on " << operation.operand1() << " and " << operation.operand2() << endl;
    }
    else {
      // Forward data from the previous instruction if necessary.
      if (operation.operand1Source() == DecodedInst::BYPASS && previousInstExecuted) {
        operation.operand1(forwardedResult);
        operation.operand1Source(DecodedInst::IMMEDIATE); // So we don't forward again.
        if (DEBUG) cout << this->name() << " forwarding contents of register "
            << (int)operation.sourceReg1() << ": " << forwardedResult << endl;
        if (ENERGY_TRACE)
          Instrumentation::Registers::forward(1);
      }
      if (operation.operand2Source() == DecodedInst::BYPASS && previousInstExecuted) {
        operation.operand2(forwardedResult);
        operation.operand2Source(DecodedInst::IMMEDIATE); // So we don't forward again.
        if (DEBUG) cout << this->name() << " forwarding contents of register "
            << (int)operation.sourceReg2() << ": " << forwardedResult << endl;
        if (ENERGY_TRACE)
          Instrumentation::Registers::forward(2);
      }

      if (DEBUG) cout << this->name() << ": executing " << operation.name()
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
        // TODO: reading the CMT just returns a network address. We want
        // the entire contents.
        operation.result(core()->channelMapTable.read(operation.operand1()).toUInt());
        break;

      case InstructionMap::OP_GETCHMAPI:
        // TODO: reading the CMT just returns a network address. We want
        // the entire contents.
        operation.result(core()->channelMapTable.read(operation.immediate()).toUInt());
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

      case InstructionMap::OP_SYSCALL:
        // TODO: remove from ALU.
        alu.systemCall(operation);
        break;

      case InstructionMap::OP_WOCHE:
        throw UnsupportedFeatureException("woche instruction");
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
  // PAYLOAD_ONLY means this is the second half of a store operation - we don't
  // want to instrument it twice.
  if (//ENERGY_TRACE &&  <-- do this check elsewhere
      operation.isALUOperation() &&
      operation.memoryOp() != MemoryRequest::PAYLOAD_ONLY &&
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
	if (Arguments::lbtTrace()) {
	  if ((currentInst.opcode() == InstructionMap::OP_LDW ||
		currentInst.opcode() == InstructionMap::OP_LDHWU ||
		currentInst.opcode() == InstructionMap::OP_LDBU ||
		currentInst.opcode() == InstructionMap::OP_STW ||
		currentInst.opcode() == InstructionMap::OP_STHW ||
		currentInst.opcode() == InstructionMap::OP_STB) && currentInst.memoryOp() != MemoryRequest::PAYLOAD_ONLY)
	  {
		LBTTrace::setInstructionMemoryAddress(currentInst.isid(), currentInst.result());
	  }
	  else if ((currentInst.opcode() == InstructionMap::OP_STW ||
		currentInst.opcode() == InstructionMap::OP_STHW ||
		currentInst.opcode() == InstructionMap::OP_STB) && currentInst.memoryOp() == MemoryRequest::PAYLOAD_ONLY)
	  {
		LBTTrace::setInstructionMemoryData(currentInst.isid(), currentInst.result());
	  }
	}

    // Memory operations may be sent to different memory banks depending on the
    // address accessed.
    // In practice, this would be performed by a separate, small functional
    // unit in parallel with the main ALU, so that there is time left to request
    // a path to memory.
    if (currentInst.isMemoryOperation())
      adjustNetworkAddress(currentInst);

    // Send the data to the output buffer - it will arrive immediately so that
    // network resources can be requested the cycle before they are used.
    oData.write(currentInst.toNetworkData());
  }

  // Send the data to the register file - it will arrive at the beginning
  // of the next clock cycle.
  outputInstruction(currentInst);
}

void ExecuteStage::setChannelMap(DecodedInst& inst) {
  MapIndex entry = inst.operand2();
  uint32_t value = inst.operand1();

  // TODO: extract these from the ChannelID, rather than from the raw data.
  ChannelIndex returnTo = (value >> 29) & 0x7UL;
  uint groupBits = (value >> 4) & 0xFUL;
  uint lineBits  = value & 0xFUL;

  // There are no free bits to encode this, so it will have to be compiled in for now.
  bool writeThrough = /*entry==2;//*/false;

  ChannelID sendChannel(value);

  // Write to the channel map table.
  // FIXME: I don't think it's necessary to block until all credits have been
  // received, but it's useful for debug purposes to ensure that we aren't
  // losing credits.
  bool success = core()->channelMapTable.write(entry, sendChannel, groupBits, lineBits, returnTo, writeThrough);
  if (!success) {
    blocked = true;
    if (DEBUG)
      cout << this->name() << " stalled waiting for credits before setchmap" << endl;
    Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_OUTPUT, inst);
    next_trigger(core()->channelMapTable.allCreditsEvent(entry));
  }
  else {
    blocked = false;
    Instrumentation::Stalls::unstall(id, Instrumentation::Stalls::STALL_OUTPUT, inst);

    // Generate a message to claim the port we have just stored the address of.
    if (sendChannel.isCore() && !sendChannel.isNullMapping()) {
      ChannelID returnChannel(id, entry);
      inst.result(returnChannel.toInt());
      inst.channelMapEntry(entry);
      inst.networkDestination(sendChannel);
      inst.portClaim(true);
      inst.usesCredits(core()->channelMapTable[entry].usesCredits());

      core()->channelMapTable[entry].removeCredit();
    }
  }
}

void ExecuteStage::adjustNetworkAddress(DecodedInst& inst) const {
  assert(inst.isMemoryOperation());
  MemoryRequest::MemoryOperation op = (MemoryRequest::MemoryOperation)inst.memoryOp();

  bool addressFlit;

  switch (op) {
    case MemoryRequest::LOAD_W:
    case MemoryRequest::LOAD_HW:
    case MemoryRequest::LOAD_B:
    case MemoryRequest::LOAD_THROUGH_W:
    case MemoryRequest::LOAD_THROUGH_HW:
    case MemoryRequest::LOAD_THROUGH_B:
    case MemoryRequest::STORE_W:
    case MemoryRequest::STORE_HW:
    case MemoryRequest::STORE_B:
    case MemoryRequest::STORE_THROUGH_W:
    case MemoryRequest::STORE_THROUGH_HW:
    case MemoryRequest::STORE_THROUGH_B:
    case MemoryRequest::IPK_READ:
      addressFlit = true;
      break;
    default:
      addressFlit = false;
      break;
  }

  // We want to access lots of information from the channel map table, so get
  // the entire entry.
  ChannelMapEntry& channelMapEntry = core()->channelMapTable[inst.channelMapEntry()];

  if (channelMapEntry.writeThrough()) {
    switch (op) {
      case MemoryRequest::STORE_W:
        op = MemoryRequest::STORE_THROUGH_W;
        break;
      case MemoryRequest::STORE_HW:
        op = MemoryRequest::STORE_THROUGH_HW;
        break;
      case MemoryRequest::STORE_B:
        op = MemoryRequest::STORE_THROUGH_B;
        break;
      case MemoryRequest::LOAD_W:
        op = MemoryRequest::LOAD_THROUGH_W;
        break;
      case MemoryRequest::LOAD_HW:
        op = MemoryRequest::LOAD_THROUGH_HW;
        break;
      case MemoryRequest::LOAD_B:
        op = MemoryRequest::LOAD_THROUGH_B;
        break;
      default:
        break;
    }
  }

  Word w = MemoryRequest(op, inst.result());

  // Adjust destination channel based on memory configuration if necessary
  uint32_t increment = 0;

  if (channelMapEntry.localMemory() && channelMapEntry.memoryGroupBits() > 0) {
    if (addressFlit) {
      increment = channelMapEntry.computeAddressIncrement((uint32_t)inst.result());

      // Store the adjustment which must be made, so that any subsequent flits
      // can also access the same memory bank.
      channelMapEntry.setAddressIncrement(increment);
    } else {
      increment = channelMapEntry.getAddressIncrement();
    }
  }

  // Set the instruction's result to the data + memory operation
  inst.result(w.toULong());

  ChannelID adjusted = inst.networkDestination();
  adjusted = adjusted.addPosition(increment);
  inst.networkDestination(adjusted);
  inst.returnAddr(channelMapEntry.returnChannel());
}

bool ExecuteStage::isStalled() const {
  // When we have multi-cycle operations (e.g. multiplies), we will need to use
  // this.
  // TODO: replace with oData.valid();
  // Currently results in occasional buffer over-filling.
  return !iReady.read();
//  return oData.valid();
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
  assert(inst.setsPredicate());

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

const DecodedInst& ExecuteStage::currentInstruction() const {
  return currentInst;
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

  previousInstExecuted = false;
  blocked = false;
  forwardedResult = 0;

  SC_METHOD(execute);
  sensitive << newInstructionEvent;
  dont_initialize();

  SC_METHOD(updateReady);
  sensitive << instructionCompletedEvent;
  // do initialise

}
