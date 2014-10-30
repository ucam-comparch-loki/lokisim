/*
 * Decoder.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "Decoder.h"
#include "DecodeStage.h"
#include "../RegisterFile.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/Instruction.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../../../Utility/Trace/CoreTrace.h"
#include "../../../Utility/Trace/LBTTrace.h"
#include "../../../Utility/InstructionMap.h"
#include "../../../Utility/Instrumentation.h"

typedef RegisterFile Registers;

// Below is a failed attempt at converting the decoder into a state machine so
// the inefficient SC_THREAD which performs decoding can be replaced with a
// better SC_METHOD. I would still like to get this done sometime, so haven't
// tidied it away.

/*
void Decoder::initialise(const DecodedInst& input) {
  current = input;

  // If we are in remote execution mode, and this instruction is marked, send it.
  if (sendChannel != Instruction::NO_CHANNEL) {
    if (current.predicate() == Instruction::P) {
      output = current;
      remoteExecution(output);
      continueToExecute = true;
      return;
    }
    // Drop out of remote execution mode when we find an unmarked instruction.
    else {
      sendChannel = Instruction::NO_CHANNEL;
      if (DEBUG) cout << this->name() << " ending remote execution" << endl;
    }
  }

  // Instrumentation stuff.
  if (ENERGY_TRACE)
    Instrumentation::decoded(id, current);
  CoreTrace::decodeInstruction(id.getPosition(), current.location(), current.endOfIPK());
}

void Decoder::collectOperands() {
  haveAllOperands = false;

  // Terminate the instruction here if it has been cancelled.
  if (instructionCancelled) {
    if (DEBUG)
      cout << this->name() << " aborting " << current << endl;
    instructionCancelled = false;
    continueToExecute = false;
    haveAllOperands = true;
    stall(false, Instrumentation::Stalls::STALL_DATA);
    return;
  }

  // Don't want to read registers if this is an instruction to be sent
  // somewhere else.
  if (sendChannel != Instruction::NO_CHANNEL) {
    haveAllOperands = true;
    return;
  }

  // Stall for one cycle if this instruction needs the predicate bit in this
  // cycle, but depends on the previous instruction to set it.
  // FIXME: preferably stall until the clock edge after the predicate bit has
  // been set - this will usually be in one cycle, but not always.
  if (needPredicateNow(output) && previous.setsPredicate()) {
    // TODO
//    next_trigger(clock.posedge_event());
    stall(true, Instrumentation::Stalls::STALL_DATA);
    next_trigger(1, sc_core::SC_NS);
    previous.setsPredicate(false);
    return;
  }

  execute = shouldExecute(output);

  // If the instruction may perform irreversible reads from a channel-end,
  // and we know it won't execute, stop it here.
  if (!execute && (Registers::isChannelEnd(output.sourceReg1()) ||
                   Registers::isChannelEnd(output.sourceReg2()))) {
    continueToExecute = false;
  }

  // If any operands are due to arrive over the network and are not here yet,
  // wait for them. next_trigger will be set if necessary.
  // FIXME: do we want to wait for all operands needed by the instruction, or
  // just the ones needed in this cycle?
  waitForOperands2(current);

  if (!haveAllOperands)
    return;

  setOperand1(output);
  setOperand2(output);
}

void Decoder::decode() {
  // No decoding to do if we're in remote execution mode.
  if (sendChannel != Instruction::NO_CHANNEL)
    return;

  // Each current instruction may spawn multiple output operations (e.g. stw
  // becomes an addition and then sends the data).
  output = current;

  // Some operations will not need to continue to the execute stage. They
  // complete all their work in the decode stage.
  continueToExecute = true;
  needDestination = needOperand1 = needOperand2 = needChannel = true;

  // Perform any operation-specific work.
  opcode_t operation = current.opcode();
  switch (operation) {

    case InstructionMap::OP_LDW:
      output.memoryOp(MemoryRequest::LOAD_W); break;
    case InstructionMap::OP_LDHWU:
      output.memoryOp(MemoryRequest::LOAD_HW); break;
    case InstructionMap::OP_LDBU:
      output.memoryOp(MemoryRequest::LOAD_B); break;

    case InstructionMap::OP_STW:
    case InstructionMap::OP_STHW:
    case InstructionMap::OP_STB: {
      // Stores are split into two sections.
      if (outputsRemaining == 0) {
        outputsRemaining = 1;
        blockedEvent.notify();
        output.endOfNetworkPacket(false);

        // The first part of a store is computing the address. This uses the
        // second source register and the immediate.
        output.sourceReg1(output.sourceReg2()); output.sourceReg2(0);

        if (operation == InstructionMap::OP_STW)
          output.memoryOp(MemoryRequest::STORE_W);
        else if (operation == InstructionMap::OP_STHW)
          output.memoryOp(MemoryRequest::STORE_HW);
        else
          output.memoryOp(MemoryRequest::STORE_B);
      }
      else {
        outputsRemaining = 0;
        blockedEvent.notify();

        previous.destination(-1);

        // The second part of a store sends the data. This uses only the first
        // source register.
        output.sourceReg2(0); output.immediate(0);
        output.memoryOp(MemoryRequest::PAYLOAD_ONLY);
      }
      break;
    }

    // lui only overwrites part of the word, so we need to read the word first.
    // Alternative: have an "lui mode" for register-writing (note that this
    // wouldn't allow data forwarding).
    case InstructionMap::OP_LUI: output.sourceReg1(output.destination()); break;

    case InstructionMap::OP_WOCHE: output.result(output.immediate()); break;

    case InstructionMap::OP_TSTCH:
    case InstructionMap::OP_TSTCHI:
    case InstructionMap::OP_TSTCH_P:
    case InstructionMap::OP_TSTCHI_P:
      output.result(parent()->testChannel(output.immediate())); break;

    case InstructionMap::OP_SELCH: output.result(parent()->selectChannel()); break;

    case InstructionMap::OP_IBJMP: continueToExecute = false; break;

    case InstructionMap::OP_IWTR: {
      output.destination(output.sourceReg1());
      break;
    }

    case InstructionMap::OP_RMTEXECUTE: {
      sendChannel = output.channelMapEntry();

      if(DEBUG) cout << this->name() << " beginning remote execution" << endl;

      continueToExecute = false;
      break;
    }

    case InstructionMap::OP_RMTNXIPK: {
      Instruction inst = current.toInstruction();
      inst.opcode(InstructionMap::OP_NXIPK);
      output.result(inst.toLong());
      break;
    }

    case InstructionMap::OP_FETCHR:
    case InstructionMap::OP_FETCHPSTR:
    case InstructionMap::OP_FILLR: {
      // Fetches implicitly access channel map table entry 0. This may change.
      output.channelMapEntry(0);
      output.memoryOp(MemoryRequest::IPK_READ);

      // The "relative" versions of these instructions have r1 (current IPK
      // address) as an implicit argument.
      output.sourceReg1(1);

      break;
    }

    case InstructionMap::OP_FETCH:
    case InstructionMap::OP_FETCHPST:
    case InstructionMap::OP_FILL:
    case InstructionMap::OP_PSEL_FETCH: {
      // Fetches implicitly access channel map table entry 0. This may change.
      output.channelMapEntry(0);
      output.memoryOp(MemoryRequest::IPK_READ);
      break;
    }

    default: break;

  } // end switch
}
*/

void Decoder::instructionFinished() {
  // If the instruction will not reach the ALU, record that it executed here.
  if (/*ENERGY_TRACE &&*/ !current.isALUOperation())
    Instrumentation::executed(id, current, execute);

  // Do ibjmps here, once we know whether or not this instruction will execute.
  if (current.opcode() == InstructionMap::OP_IBJMP) {
    JumpOffset jump = (JumpOffset)current.immediate();

    // If we know that this jump is going to execute, and the fetch stage has
    // already provided the next instruction, discard the instruction and
    // change the amount we need to jump by.
    if (execute) {
      bool discarded = discardNextInst();
      if(discarded) jump -= BYTES_PER_WORD;
      parent()->jump(jump);
    }
  }

  // Store the previous instruction so we can determine whether forwarding will
  // be required, etc. In practice, we'd only need a subset of the information.
  previous = current;

  // Avoid forwarding data from instructions which don't execute or don't store
  // their results.
  if (!execute || previous.destination() == 0)
    previous.destination(-1);
}

bool Decoder::allOperandsReady() const {
  return haveAllOperands;
}

bool Decoder::hasOutput() const {
  return continueToExecute;
}

const DecodedInst& Decoder::getOutput() const {
  return output;
}

bool Decoder::decodeInstruction(const DecodedInst& input, DecodedInst& output) {
  // If this operation produces multiple outputs, produce the next one.
  if (multiCycleOp) return continueOp(input, output);

  // In practice, we wouldn't be receiving a decoded instruction, and the
  // decode would be done here, but it simplifies various interfaces if it is
  // done this way.
  output = input;

  // If we are in remote execution mode, and this instruction is marked, send it.
  if (sendChannel != Instruction::NO_CHANNEL) {
    if (input.predicate() == Instruction::P) {
      remoteExecution(output);
      return true;
    }
    // Drop out of remote execution mode when we find an unmarked instruction.
    else {
      sendChannel = Instruction::NO_CHANNEL;
      if (DEBUG) cout << this->name() << " ending remote execution" << endl;
    }
  }

  // Determine if this instruction should execute. This may require waiting for
  // the previous instruction to set the predicate bit if this instruction
  // is completed in the decode stage, or if the instruction may perform an
  // irreversible channel read.
  // TODO: avoid any stalls or other changes if execute is false.
  bool execute = shouldExecute(input);

  if (ENERGY_TRACE)
    Instrumentation::decoded(id, input);

  CoreTrace::decodeInstruction(id.getPosition(), input.location(), input.endOfIPK());

  if (LBT_TRACE) {
	LBTTrace::LBTOperationType opType;

    switch (input.opcode()) {
	case InstructionMap::OP_LDW:
		opType = LBTTrace::LoadWord;
		break;

	case InstructionMap::OP_LDHWU:
		opType = LBTTrace::LoadHalfWord;
		break;

	case InstructionMap::OP_LDBU:
		opType = LBTTrace::LoadByte;
		break;

	case InstructionMap::OP_STW:
		opType = LBTTrace::StoreWord;
		break;

	case InstructionMap::OP_STHW:
		opType = LBTTrace::StoreHalfWord;
		break;

	case InstructionMap::OP_STB:
		opType = LBTTrace::StoreByte;
		break;

	case InstructionMap::OP_FETCH:
	case InstructionMap::OP_FETCHR:
	case InstructionMap::OP_FETCHPST:
	case InstructionMap::OP_FETCHPSTR:
	case InstructionMap::OP_FILL:
	case InstructionMap::OP_FILLR:
	case InstructionMap::OP_PSEL_FETCH:
	case InstructionMap::OP_PSEL_FETCHR:
		opType = LBTTrace::Fetch;
		break;

	case InstructionMap::OP_IWTR:
		opType = LBTTrace::ScratchpadWrite;
		break;

	case InstructionMap::OP_IRDR:
		opType = LBTTrace::ScratchpadRead;
		break;

	case InstructionMap::OP_LLI:
	case InstructionMap::OP_LUI:
		opType = LBTTrace::LoadImmediate;
		break;

	case InstructionMap::OP_SYSCALL:
		opType = LBTTrace::SystemCall;
		break;

	default:
		if (InstructionMap::isALUOperation(input.opcode())) {
			if (input.function() == InstructionMap::FN_MULHW || input.function() == InstructionMap::FN_MULLW)
				opType = LBTTrace::ALU2;
			else
				opType = LBTTrace::ALU1;
		} else {
			opType = LBTTrace::Control;
		}

		break;
    }

	unsigned long long isid = LBTTrace::logDecodedInstruction(input.location(), opType, input.endOfIPK());

	LBTTrace::setInstructionExecuteFlag(isid, execute);
	LBTTrace::setInstructionInputChannels(isid, input.sourceReg1() - 2, Registers::isChannelEnd(input.sourceReg1()), input.sourceReg2() - 2, Registers::isChannelEnd(input.sourceReg2()));

	input.isid(isid);
	output.isid(isid);
  }

  if (/*ENERGY_TRACE &&*/ !input.isALUOperation())
    Instrumentation::executed(id, input, execute);

  // If the instruction may perform irreversible reads from a channel-end,
  // and we know it won't execute, stop it here.
  if (!execute && (Registers::isChannelEnd(input.sourceReg1()) ||
                   Registers::isChannelEnd(input.sourceReg2()))) {
    return true;
  }

  // Wait for any unavailable operands to arrive (either over the network, or
  // by being forwarded from the execute stage).
  waitForOperands(input);

  // Terminate the instruction here if it has been cancelled.
  if (instructionCancelled) {
    if (DEBUG)
      cout << this->name() << " aborting " << input << endl;
    instructionCancelled = false;

    if (LBT_TRACE)
   	  LBTTrace::setInstructionExecuteFlag(input.isid(), false);

    return false;
  }

  // Some operations will not need to continue to the execute stage. They
  // complete all their work in the decode stage.
  bool continueToExecute = true;

  // Perform any operation-specific work.
  opcode_t operation = input.opcode();
  switch (operation) {

    case InstructionMap::OP_LDW:
      output.memoryOp(MemoryRequest::LOAD_W); break;
    case InstructionMap::OP_LDHWU:
      // FIXME: horrible hack - artificial memory latency for load-throughs.
      /*if (input.channelMapEntry() == 2) {
        blocked = true; blockedEvent.notify();
        wait(MEMORY_ON_CHIP_SCRATCHPAD_DELAY, sc_core::SC_NS);
        blocked = false; blockedEvent.notify();
      }*/
      output.memoryOp(MemoryRequest::LOAD_HW); break;
    case InstructionMap::OP_LDBU:
      output.memoryOp(MemoryRequest::LOAD_B); break;

    case InstructionMap::OP_STW:
    case InstructionMap::OP_STHW:
    case InstructionMap::OP_STB: {
      multiCycleOp = true;
      output.endOfNetworkPacket(false);
      blockedEvent.notify();

      // The first part of a store is computing the address. This uses the
      // second source register and the immediate.
      output.sourceReg1(output.sourceReg2()); output.sourceReg2(0);

      if (operation == InstructionMap::OP_STW)
        output.memoryOp(MemoryRequest::STORE_W);
      else if (operation == InstructionMap::OP_STHW)
        output.memoryOp(MemoryRequest::STORE_HW);
      else
        output.memoryOp(MemoryRequest::STORE_B);
      break;
    }

    case InstructionMap::OP_IRDR:
      multiCycleOp = true;
      blockedEvent.notify();

      // Read register
      setOperand1(output);

      // Wait for result from execute stage, if necessary.
      if (output.operand1Source() == DecodedInst::BYPASS) {
        Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_FORWARDING);
        wait(1.1, sc_core::SC_NS);
        Instrumentation::Stalls::unstall(id, Instrumentation::Stalls::STALL_FORWARDING);
        output.operand1(parent()->getForwardedData());
      }
      output.sourceReg1(output.operand1());

      // Wait a cycle before reading again (+0.1 to ensure other things happen first).
      wait(1.1, sc_core::SC_NS);
      multiCycleOp = false;
      blockedEvent.notify();
      waitForOperands(output);
      // The second register read happens as usual in setOperand1, below.
      break;

    // lui only overwrites part of the word, so we need to read the word first.
    // Alternative: have an "lui mode" for register-writing (note that this
    // wouldn't allow data forwarding).
    case InstructionMap::OP_LUI: output.sourceReg1(output.destination()); break;

    case InstructionMap::OP_WOCHE: output.result(output.immediate()); break;

    case InstructionMap::OP_TSTCHI:
    case InstructionMap::OP_TSTCHI_P:
      output.result(parent()->testChannel(output.immediate())); break;

    case InstructionMap::OP_SELCH:
      output.result(parent()->selectChannel(output.immediate())); break;

    case InstructionMap::OP_IBJMP: {
      JumpOffset jump = (JumpOffset)output.immediate();

      // If we know that this jump is going to execute, and the fetch stage has
      // already provided the next instruction, discard the instruction and
      // change the amount we need to jump by.
      if (execute) {
        bool discarded = discardNextInst();
        if (discarded)
          jump--;
        parent()->jump(jump);
      }

      continueToExecute = false;
      break;
    }

    case InstructionMap::OP_RMTEXECUTE: {
      sendChannel = output.channelMapEntry();

      if (DEBUG)
        cout << this->name() << " beginning remote execution" << endl;

      continueToExecute = false;
      break;
    }

    case InstructionMap::OP_RMTNXIPK: {
      Instruction inst = input.toInstruction();
      inst.opcode(InstructionMap::OP_NXIPK);
      output.result(inst.toLong());
      break;
    }

    case InstructionMap::OP_FETCHR:
    case InstructionMap::OP_FETCHPSTR:
    case InstructionMap::OP_FILLR:
    case InstructionMap::OP_PSEL_FETCHR:
    case InstructionMap::OP_FETCH:
    case InstructionMap::OP_FETCHPST:
    case InstructionMap::OP_FILL:
    case InstructionMap::OP_PSEL_FETCH:
      continueToExecute = false;
      break;

    default: break;

  } // end switch

  // Gather the operands this instruction needs.
  setOperand1(output);
  setOperand2(output);

  if (execute && isFetch(output.opcode()))
    fetch(output);

  // Store the instruction we just decoded so we can see if it needs to forward
  // data to the next instruction. In practice, we wouldn't need the whole
  // instruction.
  previous = output;

  // Don't want to forward from instructions which didn't execute, or which
  // don't store their results.
  if (!execute || input.destination() == 0)
    previous.destination(-1);

  return continueToExecute;
}

bool Decoder::ready() const {
  return !multiCycleOp && !blocked && outputsRemaining == 0;
}

void Decoder::waitForOperands(const DecodedInst& dec) {
  // Wait until all data has arrived from the network.
  if (InstructionMap::hasDestReg(dec.opcode()) && Registers::isChannelEnd(dec.destination()))
    waitUntilArrival(Registers::toChannelID(dec.destination()));
  if (InstructionMap::hasSrcReg1(dec.opcode())) {
    if (Registers::isChannelEnd(dec.sourceReg1()))
      waitUntilArrival(Registers::toChannelID(dec.sourceReg1()));
    if (!dec.isALUOperation() && !dec.isMemoryOperation() && (dec.sourceReg1() == previous.destination())) {
      Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_FORWARDING);
      // HACK! May take multiple cycles. FIXME
      // Add an extra 0.1 cycles to ensure that the result is ready for forwarding.
      // Would ideally like to use execute.executedEvent(), but then also need
      // to check for whether the instruction has already executed.
      wait(1.1, sc_core::SC_NS);
      Instrumentation::Stalls::unstall(id, Instrumentation::Stalls::STALL_FORWARDING);
    }
  }
  if (InstructionMap::hasSrcReg2(dec.opcode()) && Registers::isChannelEnd(dec.sourceReg2()))
    waitUntilArrival(Registers::toChannelID(dec.sourceReg2()));

  // FIXME: there is currently a problem if we are indirectly reading from a
  // channel end, and the instruction is aborted. This method doesn't wait for
  // registers specified by indirect reads.
}

void Decoder::setOperand1(DecodedInst& dec) {
  RegisterIndex reg = dec.sourceReg1();

  if ((reg == previous.destination())) {
    dec.operand1Source(DecodedInst::BYPASS);
    if (previous.usesPredicate())
      dec.operand1(readRegs(1, reg));
  }
  else {
    dec.operand1(readRegs(1, reg));
    dec.operand1Source(DecodedInst::REGISTER);
  }
}

void Decoder::setOperand2(DecodedInst& dec) {
  if (dec.hasImmediate()) {
    dec.operand2(dec.immediate());
    dec.operand2Source(DecodedInst::IMMEDIATE);
  }
  else if (dec.hasSrcReg2()) {
    RegisterIndex reg = dec.sourceReg2();

    if (reg == previous.destination()) {
      dec.operand2Source(DecodedInst::BYPASS);
      if (previous.usesPredicate())
        dec.operand2(readRegs(2, reg));
    }
    else {
      dec.operand2(readRegs(2, reg));
      dec.operand2Source(DecodedInst::REGISTER);
    }
  }
}

int32_t Decoder::readRegs(PortIndex port, RegisterIndex index, bool indirect) {
  return parent()->readReg(port, index, indirect);
}

void Decoder::waitUntilArrival(ChannelIndex channel) {
  if (instructionCancelled)
    return;

  // Test the channel to see if the data is already there.
  if (!parent()->testChannel(channel)) {
    stall(true, Instrumentation::Stalls::STALL_DATA);

    if (DEBUG)
      cout << this->name() << " waiting for channel " << (int)channel << endl;

    // Wait until something arrives.
    wait(parent()->receivedDataEvent(channel) | cancelEvent);

    stall(false, Instrumentation::Stalls::STALL_DATA);
  }
}

void Decoder::waitForOperands2(const DecodedInst& inst) {
  haveAllOperands = false;

  // Hack: simulate a bypass path in the channel map table here. Bypassing is
  // awkward to implement efficiently in SystemC.
  if (needChannel && inst.networkDestination() != Instruction::NO_CHANNEL) {
    needChannel = false;
    if ((previous.opcode() == InstructionMap::OP_SETCHMAP ||
         previous.opcode() == InstructionMap::OP_SETCHMAPI)  &&
        (previous.immediate() == inst.channelMapEntry())) {
      next_trigger(sc_core::SC_ZERO_TIME);
      return;
    }
  }

  // For each of the three possible registers described by the instruction:
  //  1. See if the register corresponds to an input channel.
  //  2. If the channel is empty, wait for data to arrive.
  if (needDestination && InstructionMap::hasDestReg(inst.opcode()) && Registers::isChannelEnd(inst.destination())) {
    needDestination = false;
    if (!checkChannelInput(Registers::toChannelID(inst.destination())))
      return;
  }
  if (needOperand1 && InstructionMap::hasSrcReg1(inst.opcode()) && Registers::isChannelEnd(inst.sourceReg1())) {
    needOperand1 = false;
    if (!checkChannelInput(Registers::toChannelID(inst.sourceReg1())))
      return;
  }
  if (needOperand2 && InstructionMap::hasSrcReg2(inst.opcode()) && Registers::isChannelEnd(inst.sourceReg2())) {
    needOperand2 = false;
    if (!checkChannelInput(Registers::toChannelID(inst.sourceReg2())))
      return;
  }

  haveAllOperands = true;

  // If the decoder blocked waiting for any input, unblock it now that all data
  // have arrived.
  if (blocked)
    stall(false, Instrumentation::Stalls::STALL_DATA);

  // FIXME: there is currently a problem if we are indirectly reading from a
  // channel end, and the instruction is aborted. This method doesn't wait for
  // registers specified by indirect reads.
}

bool Decoder::checkChannelInput(ChannelIndex channel) {
  bool haveData = parent()->testChannel(channel);

  if (!haveData) {
    stall(true, Instrumentation::Stalls::STALL_DATA);  // Remember to unstall again afterwards.

    if (DEBUG)
      cout << this->name() << " waiting for channel " << (int)channel << endl;

    next_trigger(parent()->receivedDataEvent(channel) | cancelEvent);
  }

  return haveData;
}

void Decoder::remoteExecution(DecodedInst& instruction) const {
  // "Re-encode" the instruction so it can be sent. In practice, it wouldn't
  // have been decoded yet, so there would be no extra work here.
  Instruction encoded = instruction.toInstruction();

  // Set the instruction to always execute
  encoded.predicate(Instruction::END_OF_PACKET);

  // The data to be sent is the instruction itself.
  instruction.result(encoded.toLong());
  instruction.channelMapEntry(sendChannel);

  // Prevent other stages from trying to execute this instruction.
  instruction.predicate(Instruction::ALWAYS);
  instruction.opcode(InstructionMap::OP_OR);
  instruction.destination(0);
}

bool Decoder::discardNextInst() const {
  return parent()->discardNextInst();
}

void Decoder::cancelInstruction() {
  if (blocked) {
    instructionCancelled = true;
    cancelEvent.notify();
  }
}

/* Sends the second part of a two-flit store operation (the data to send). */
bool Decoder::continueOp(const DecodedInst& input, DecodedInst& output) {
  output = input;

  // Store operations send the address in the first cycle, and the data in
  // the second.
  setOperand1(output);
  output.memoryOp(MemoryRequest::PAYLOAD_ONLY);

  multiCycleOp = false;
  blockedEvent.notify();
  
  return true;
}

// Use the predicate to determine if the instruction should execute or not.
// Most instructions will only need to do this check in the execute stage,
// when the predicate is guaranteed to be up to date. The exceptions are:
//  * Instructions which complete in the decode stage
//  * Instructions which cannot be undone (e.g. reading from channel ends)
//
// An optimisation would be to perform the predicate check (and possibly
// avoid reading registers) if we know that the previous instruction won't
// change the predicate.
bool Decoder::shouldExecute(const DecodedInst& inst) {

  // Instructions which will always execute.
  if (!inst.usesPredicate() || inst.opcode() == InstructionMap::OP_PSEL_FETCH
                            || inst.opcode() == InstructionMap::OP_PSEL_FETCHR
                            || inst.opcode() == InstructionMap::OP_PSEL)
    return true;

  // Predicated instructions which complete in this pipeline stage and
  // which access channel data.
  if ((!inst.isALUOperation() && !inst.isMemoryOperation()) ||
      isFetch(inst.opcode()) ||
      Registers::isChannelEnd(inst.sourceReg1()) ||
      Registers::isChannelEnd(inst.sourceReg2())) {
    short predBits = inst.predicate();
    bool predicate = parent()->predicate();

    return (predBits == Instruction::P     &&  predicate) ||
           (predBits == Instruction::NOT_P && !predicate);
  }
  // All other predicated instructions.
  else
    return true;
}

// Tell whether we need to know the value of the predicate register in this
// clock cycle. Most instructions wait until the execute stage to access it,
// but some will perform irreversible operations here, so need to be
// stopped if necessary.
bool Decoder::needPredicateNow(const DecodedInst& inst) const {
  bool predicated = inst.predicate() == Instruction::P ||
                    inst.predicate() == Instruction::NOT_P;
  bool irreversible = (!inst.isALUOperation() && !inst.isMemoryOperation()) ||
                      isFetch(inst.opcode()) ||
                      Registers::isChannelEnd(inst.sourceReg1()) ||
                      Registers::isChannelEnd(inst.sourceReg2());
  return predicated && irreversible;
}

bool Decoder::isFetch(opcode_t opcode) const {
  bool fetch;

  switch (opcode) {
    case InstructionMap::OP_FETCHR:
    case InstructionMap::OP_FETCHPSTR:
    case InstructionMap::OP_FILLR:
    case InstructionMap::OP_PSEL_FETCHR:
    case InstructionMap::OP_FETCH:
    case InstructionMap::OP_FETCHPST:
    case InstructionMap::OP_FILL:
    case InstructionMap::OP_PSEL_FETCH:
      fetch = true;
      break;
    default:
      fetch = false;
      break;
  }

  return fetch;
}

void Decoder::fetch(DecodedInst& inst) {
  // Wait until we are allowed to check the cache tags.
  if (!parent()->canFetch()) {
    // TODO: would prefer a better stall reason.
    stall(true, Instrumentation::Stalls::STALL_INSTRUCTIONS);
    while (!parent()->canFetch()) {
      if (DEBUG)
        cout << this->name() << " waiting to issue " << inst << endl;
      wait(1, sc_core::SC_NS);
    }
    stall(false, Instrumentation::Stalls::STALL_INSTRUCTIONS);
  }

  // Extra forwarding path.
  if (inst.operand1Source() == DecodedInst::BYPASS ||
      inst.operand2Source() == DecodedInst::BYPASS) {
    stall(true, Instrumentation::Stalls::STALL_FORWARDING);
    wait(1.1, sc_core::SC_NS);
    stall(false, Instrumentation::Stalls::STALL_FORWARDING);

    if (inst.operand1Source() == DecodedInst::BYPASS)
      inst.operand1(readRegs(1, inst.sourceReg1()));
    if (inst.operand2Source() == DecodedInst::BYPASS)
      inst.operand2(readRegs(2, inst.sourceReg2()));
  }

  parent()->fetch(inst);
}

void Decoder::stall(bool stall, Instrumentation::Stalls::StallReason reason) {
  blocked = stall;
  if (stall)
    Instrumentation::Stalls::stall(id, reason);
  else
    Instrumentation::Stalls::unstall(id, reason);
  blockedEvent.notify();
}

const sc_event& Decoder::stalledEvent() const {
  return blockedEvent;
}

DecodeStage* Decoder::parent() const {
  return static_cast<DecodeStage*>(this->get_parent_object());
}

void Decoder::reportStalls(ostream& os) {
  if (!ready()) {
    DecodedInst& inst = parent()->currentInst;
    os << this->name() << " unable to complete " << inst << endl;

    if (needOperand1 || (inst.hasOperand1() && Registers::isChannelEnd(inst.sourceReg1())))
      os << "  Waiting for operand 1." << endl;
    if (needOperand2 || (inst.hasOperand2() && Registers::isChannelEnd(inst.sourceReg2())))
      os << "  Waiting for operand 2." << endl;
    if (multiCycleOp)
      os << "  Operation takes multiple cycles." << endl;
  }
}

Decoder::Decoder(const sc_module_name& name, const ComponentID& ID) :
	  Component(name, ID) {

  continueToExecute = false;
  execute = false;
  haveAllOperands = true;
  outputsRemaining = 0;
  needDestination = false;
  needOperand1 = false;
  needOperand2 = false;
  needChannel = false;

  sendChannel = Instruction::NO_CHANNEL;
  multiCycleOp = false;
  blocked = false;
  instructionCancelled = false;
  previousInstPredicated = false;
  previousInstSetPredicate = false;

}
