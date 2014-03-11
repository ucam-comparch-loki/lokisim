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

bool ExecuteStage::readPredicate() const {return parent()->readPredReg();}
int32_t ExecuteStage::readReg(RegisterIndex reg) const {return parent()->readReg(1, reg);}
int32_t ExecuteStage::readWord(MemoryAddr addr) const {return parent()->readWord(addr).toInt();}
int32_t ExecuteStage::readByte(MemoryAddr addr) const {return parent()->readByte(addr).toInt();}

void ExecuteStage::writePredicate(bool val) const {parent()->writePredReg(val);}
void ExecuteStage::writeReg(RegisterIndex reg, Word data) const {parent()->writeReg(reg, data.toInt());}
void ExecuteStage::writeWord(MemoryAddr addr, Word data) const {parent()->writeWord(addr, data);}
void ExecuteStage::writeByte(MemoryAddr addr, Word data) const {parent()->writeByte(addr, data);}

void ExecuteStage::execute() {
//  if (alu.busy()) {
//    if (DEBUG) cout << this->name() << ": continuing " << currentInst.name()
//        << " on " << currentInst.operand1() << " and " << currentInst.operand2() << endl;
//    alu.execute(currentInst);
//    blocked = alu.busy();
//  }

  if (currentInst.hasResult())
    sendOutput();             // If there is already a result, don't do anything
  else
    newInput(currentInst);

  forwardedResult = currentInst.result();

  if (!blocked)
    instructionCompleted();
}

void ExecuteStage::updateReady() {
  bool ready = !isStalled();

  // Write our current stall status.
  if (ready != readyOut.read())
    readyOut.write(ready);

  if (DEBUG && isStalled() && readyOut.read())
    cout << this->name() << " stalled." << endl;
}

void ExecuteStage::newInput(DecodedInst& operation) {
  // See if the instruction should execute.
  bool willExecute = checkPredicate(operation);

  if (LBT_TRACE)
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

      case InstructionMap::OP_FETCH:
      case InstructionMap::OP_FETCHR:
      case InstructionMap::OP_FETCHPST:
      case InstructionMap::OP_FETCHPSTR:
      case InstructionMap::OP_FILL:
      case InstructionMap::OP_FILLR:
      case InstructionMap::OP_PSEL_FETCH:
        success = fetch(operation);
        break;

      case InstructionMap::OP_IWTR:
        // Send only lowest 8 bits of address - don't need mask in hardware.
        scratchpad.write(operation.operand1() & 0xFF, operation.operand2());
        break;

      case InstructionMap::OP_IRDR:
        // Send only lowest 8 bits of address - don't need mask in hardware.
        operation.result(scratchpad.read(operation.operand1() & 0xFF));
        break;

      case InstructionMap::OP_LLI:
        // FIXME: should LLI happen in execute stage or during register write?
        operation.result(operation.operand2());
        break;

      case InstructionMap::OP_LUI:
        operation.result((operation.operand1() & 0xFFFF) | (operation.operand2() << 16));
        break;

      case InstructionMap::OP_SYSCALL:
        alu.systemCall(operation);
        break;

      case InstructionMap::OP_WOCHE:
        cerr << "woche not yet implemented" << endl;
        assert(false);
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

    if (success)
      sendOutput();

  } // end if will execute
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
      !blocked)
    Instrumentation::executed(id, operation, willExecute);

  previousInstExecuted = willExecute;
}

void ExecuteStage::sendOutput() {
  if (currentInst.sendsOnNetwork()) {
	if (LBT_TRACE) {
	  // FIXME: I am not sure whether I understood the calculation of memory addresses completely - set them here to get final result

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
    dataOut.write(currentInst);
  }

  // Send the data to the register file - it will arrive at the beginning
  // of the next clock cycle.
  outputInstruction(currentInst);
}

bool ExecuteStage::canCheckTags() const {
  return parent()->canCheckTags();
}

bool ExecuteStage::fetch(DecodedInst& inst) {
  MemoryAddr fetchAddress;

  // If we already have the maximum number of packets queued up, wait until
  // one of them finishes arriving.
  if (!canCheckTags()) {
    blocked = true;
    currentInst = inst;
    next_trigger(clock.posedge_event());
    return false;
  }
  else
    blocked = false;

  // Compute the address to fetch from, depending on which operation this is.
  switch (inst.opcode()) {
    case InstructionMap::OP_FETCH:
    case InstructionMap::OP_FETCHR:
    case InstructionMap::OP_FETCHPST:
    case InstructionMap::OP_FETCHPSTR:
    case InstructionMap::OP_FILL:
    case InstructionMap::OP_FILLR:
      fetchAddress = inst.operand1() + inst.operand2();
      break;

    case InstructionMap::OP_PSEL_FETCH:
      fetchAddress = readPredicate() ? inst.operand1() : inst.operand2();
      break;

    default:
      cerr << "Error: called ExecuteStage::fetch with non-fetch instruction." << endl;
      assert(false);
      break;
  }

  if (LBT_TRACE)
	LBTTrace::setInstructionMemoryAddress(inst.isid(), fetchAddress);

  // Return whether the fetch request should be sent (it should be sent if the
  // tag is not in the cache).
  if (parent()->inCache(fetchAddress, inst.opcode())) {
    // Packet is already in the cache: don't fetch.
    return false;
  }
  else {
    // Packet is not in the cache: fetch.
    MemoryRequest request(MemoryRequest::IPK_READ, fetchAddress);
    inst.result(request.toULong());
    return true;
  }
}

void ExecuteStage::setChannelMap(DecodedInst& inst) {
  MapIndex entry = inst.immediate();
  uint32_t value = inst.operand1();

  // Extract all of the relevant sections from the encoded destination, and
  // create a ChannelID out of them.
  // TODO: move all of this computation to within ChannelMapEntry.
  ChannelIndex returnTo = (value >> 29) & 0x7UL;
  bool multicast = (value >> 28) & 0x1UL;
  // Hack: tile shouldn't be necessary for multicast addresses
  uint tile      = multicast ? id.getTile() : ((value >> 20) & 0xFFUL);
  uint position  = (value >> 12) & 0xFFUL;
  uint channel   = (value >> 8) & 0xFUL;
  uint groupBits = (value >> 4) & 0xFUL;
  uint lineBits  = value & 0xFUL;

  ChannelID sendChannel(tile, position, channel, multicast);

  // Write to the channel map table.
  // FIXME: I don't think it's necessary to block until all credits have been
  // received, but it's useful for debug purposes to ensure that we aren't
  // losing credits.
  bool success = parent()->channelMapTable.write(entry, sendChannel, groupBits, lineBits, returnTo);
  if (!success) {
    blocked = true;
    next_trigger(parent()->channelMapTable.haveAllCredits(entry));
  }
  else {
    blocked = false;

    // Generate a message to claim the port we have just stored the address of.
    if (sendChannel.isCore()) {
      ChannelID returnChannel(id, entry);
      inst.result(returnChannel.toInt());
      inst.channelMapEntry(entry);
      inst.networkDestination(sendChannel);
      inst.portClaim(true);
      inst.usesCredits(parent()->channelMapTable[entry].usesCredits());
    }
  }
}

void ExecuteStage::adjustNetworkAddress(DecodedInst& inst) const {
  assert(inst.isMemoryOperation());
  MemoryRequest::MemoryOperation op = (MemoryRequest::MemoryOperation)inst.memoryOp();

  Word w = MemoryRequest(op, inst.result());
  bool addressFlit = op == MemoryRequest::LOAD_B ||
                     op == MemoryRequest::LOAD_HW ||
                     op == MemoryRequest::LOAD_W ||
                     op == MemoryRequest::STORE_B ||
                     op == MemoryRequest::STORE_HW ||
                     op == MemoryRequest::STORE_W ||
                     op == MemoryRequest::IPK_READ;

  // Adjust destination channel based on memory configuration if necessary
  uint32_t increment = 0;

  // We want to access lots of information from the channel map table, so get
  // the entire entry.
  ChannelMapEntry& channelMapEntry = parent()->channelMapTable[inst.channelMapEntry()];

  if (channelMapEntry.localMemory() && channelMapEntry.memoryGroupBits() > 0) {
    if (addressFlit) {
      increment = (uint32_t)inst.result();
      increment &= (1UL << (channelMapEntry.memoryGroupBits() + channelMapEntry.memoryLineBits())) - 1UL;
      increment >>= channelMapEntry.memoryLineBits();

      // Store the adjustment which must be made, so that any subsequent flits
      // can also access the same memory.
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
  return !readyIn.read();
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

const DecodedInst& ExecuteStage::currentInstruction() const {
  return currentInst;
}

const sc_event& ExecuteStage::executedEvent() const {
  return instructionCompletedEvent;
}

ExecuteStage::ExecuteStage(sc_module_name name, const ComponentID& ID) :
    PipelineStage(name, ID),
    alu("alu", ID),
    scratchpad("scratchpad", ID) {

  previousInstExecuted = false;
  blocked = false;

  SC_METHOD(execute);
  sensitive << newInstructionEvent;
  dont_initialize();

  SC_METHOD(updateReady);
  sensitive << instructionCompletedEvent;
  // do initialise

}
