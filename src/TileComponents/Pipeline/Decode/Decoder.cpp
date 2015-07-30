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

void Decoder::instructionFinished() {
  // If the instruction will not reach the ALU, record that it executed here.
  if (/*ENERGY_TRACE &&*/ !current.isExecuteStageOperation())
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

  // Determine if this instruction should execute. This may require waiting for
  // the previous instruction to set the predicate bit if this instruction
  // is completed in the decode stage, or if the instruction may perform an
  // irreversible channel read.
  bool execute = shouldExecute(input);

  Instrumentation::decoded(id, input);

  CoreTrace::decodeInstruction(id.position, input.location(), input.endOfIPK());

  if (Arguments::lbtTrace()) {
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

  if (/*ENERGY_TRACE &&*/ !input.isExecuteStageOperation()) {
    Instrumentation::executed(id, input, execute);
    parent()->instructionExecuted();
  }

  // If we know the instruction won't execute, stop it here.
  if (!execute) {
    // Disallow forwarding from instructions which didn't execute.
    previous = input;
    previous.destination(-1);
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

    if (Arguments::lbtTrace())
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
      output.memoryOp(LOAD_W); break;
    case InstructionMap::OP_LDHWU:
      output.memoryOp(LOAD_HW); break;
    case InstructionMap::OP_LDBU:
      output.memoryOp(LOAD_B); break;
    case InstructionMap::OP_LDL:
      output.memoryOp(LOAD_LINKED); break;

    case InstructionMap::OP_STW:
    case InstructionMap::OP_STHW:
    case InstructionMap::OP_STB:
    case InstructionMap::OP_STC:
    case InstructionMap::OP_LDADD:
    case InstructionMap::OP_LDOR:
    case InstructionMap::OP_LDAND:
    case InstructionMap::OP_LDXOR:
    case InstructionMap::OP_EXCHANGE: {
      multiCycleOp = true;
      output.endOfNetworkPacket(false);
      blockedEvent.notify();

      // The first part of a store is computing the address. This uses the
      // second source register and the immediate.
      output.sourceReg1(output.sourceReg2()); output.sourceReg2(0);

      switch (operation) {
        case InstructionMap::OP_STW:
          output.memoryOp(STORE_W); break;
        case InstructionMap::OP_STHW:
          output.memoryOp(STORE_HW); break;
        case InstructionMap::OP_STB:
          output.memoryOp(STORE_B); break;
        case InstructionMap::OP_STC:
          output.memoryOp(STORE_CONDITIONAL); break;
        case InstructionMap::OP_LDADD:
          output.memoryOp(LOAD_AND_ADD); break;
        case InstructionMap::OP_LDOR:
          output.memoryOp(LOAD_AND_OR); break;
        case InstructionMap::OP_LDAND:
          output.memoryOp(LOAD_AND_AND); break;
        case InstructionMap::OP_LDXOR:
          output.memoryOp(LOAD_AND_XOR); break;
        case InstructionMap::OP_EXCHANGE:
          output.memoryOp(EXCHANGE); break;
        default:
          break;
      }

      break;
    }

    case InstructionMap::OP_IRDR:
      multiCycleOp = true;
      blockedEvent.notify();

      // Read register
      setOperand1(output);

      // Wait for result from execute stage, if necessary.
      if (output.operand1Source() == DecodedInst::BYPASS) {
        stall(true, Instrumentation::Stalls::STALL_FORWARDING, output);
        wait(1.1, sc_core::SC_NS);
        stall(false, Instrumentation::Stalls::STALL_FORWARDING, output);
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

    // Pause execution until the specified channel has at least the given
    // number of credits.
    case InstructionMap::OP_WOCHE: {
      uint targetCredits = output.immediate();
      ChannelMapEntry& cme = parent()->channelMapTableEntry(output.channelMapEntry());
      while (!cme.haveNCredits(targetCredits)) {
        stall(true, Instrumentation::Stalls::STALL_OUTPUT, output);
        wait(cme.creditArrivedEvent());
      }
      stall(false, Instrumentation::Stalls::STALL_OUTPUT, output);
      continueToExecute = false;
      break;
    }

    case InstructionMap::OP_TSTCHI:
    case InstructionMap::OP_TSTCHI_P:
      output.result(parent()->testChannel(output.immediate())); break;

    case InstructionMap::OP_SELCH:
      output.result(parent()->selectChannel(output.immediate(), output)); break;

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
      parent()->startRemoteExecution(output);
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
    waitUntilArrival(Registers::toChannelID(dec.destination()), dec);
  if (InstructionMap::hasSrcReg1(dec.opcode())) {
    if (Registers::isChannelEnd(dec.sourceReg1()))
      waitUntilArrival(Registers::toChannelID(dec.sourceReg1()), dec);
    if (!dec.isExecuteStageOperation() && !dec.isMemoryOperation() && (dec.sourceReg1() == previous.destination())) {
      stall(true, Instrumentation::Stalls::STALL_FORWARDING, dec);
      // HACK! May take multiple cycles. FIXME
      // Add an extra 0.1 cycles to ensure that the result is ready for forwarding.
      // Would ideally like to use execute.executedEvent(), but then also need
      // to check for whether the instruction has already executed.
      wait(1.1, sc_core::SC_NS);
      stall(false, Instrumentation::Stalls::STALL_FORWARDING, dec);
    }
  }
  if (InstructionMap::hasSrcReg2(dec.opcode()) && Registers::isChannelEnd(dec.sourceReg2()))
    waitUntilArrival(Registers::toChannelID(dec.sourceReg2()), dec);

  // FIXME: there is currently a problem if we are indirectly reading from a
  // channel end, and the instruction is aborted. This method doesn't wait for
  // registers specified by indirect reads.
}

void Decoder::setOperand1(DecodedInst& dec) {
  RegisterIndex reg = dec.sourceReg1();

  if ((reg == previous.destination())) {
    dec.operand1Source(DecodedInst::BYPASS);
    if (previous.predicated())
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
      if (previous.predicated())
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

void Decoder::waitUntilArrival(ChannelIndex channel, const DecodedInst& inst) {
  if (instructionCancelled)
    return;

  // Test the channel to see if the data is already there.
  if (!parent()->testChannel(channel)) {
    bool fromMemory = connectionFromMemory(channel);
    Instrumentation::Stalls::StallReason reason =
        fromMemory ? Instrumentation::Stalls::STALL_MEMORY_DATA
                   : Instrumentation::Stalls::STALL_CORE_DATA;

    stall(true, reason, inst);

    if (DEBUG)
      cout << this->name() << " waiting for channel " << (int)channel << endl;

    // Wait until something arrives.
    wait(parent()->receivedDataEvent(channel) | cancelEvent);

    stall(false, reason, inst);
  }
}

void Decoder::waitForOperands2(const DecodedInst& inst) {
  haveAllOperands = false;

  // For each of the three possible registers described by the instruction:
  //  1. See if the register corresponds to an input channel.
  //  2. If the channel is empty, wait for data to arrive.
  if (needDestination && InstructionMap::hasDestReg(inst.opcode()) && Registers::isChannelEnd(inst.destination())) {
    needDestination = false;
    if (!checkChannelInput(Registers::toChannelID(inst.destination()), inst))
      return;
  }
  if (needOperand1 && InstructionMap::hasSrcReg1(inst.opcode()) && Registers::isChannelEnd(inst.sourceReg1())) {
    needOperand1 = false;
    if (!checkChannelInput(Registers::toChannelID(inst.sourceReg1()), inst))
      return;
  }
  if (needOperand2 && InstructionMap::hasSrcReg2(inst.opcode()) && Registers::isChannelEnd(inst.sourceReg2())) {
    needOperand2 = false;
    if (!checkChannelInput(Registers::toChannelID(inst.sourceReg2()), inst))
      return;
  }

  haveAllOperands = true;

  // If the decoder blocked waiting for any input, unblock it now that all data
  // have arrived.
  if (blocked) {
    stall(false, Instrumentation::Stalls::STALL_MEMORY_DATA, inst);
    stall(false, Instrumentation::Stalls::STALL_CORE_DATA, inst);
  }

  // FIXME: there is currently a problem if we are indirectly reading from a
  // channel end, and the instruction is aborted. This method doesn't wait for
  // registers specified by indirect reads.
}

bool Decoder::checkChannelInput(ChannelIndex channel, const DecodedInst& inst) {
  bool haveData = parent()->testChannel(channel);

  if (!haveData) {
    bool fromMemory = connectionFromMemory(channel);
    Instrumentation::Stalls::StallReason reason =
        fromMemory ? Instrumentation::Stalls::STALL_MEMORY_DATA
                   : Instrumentation::Stalls::STALL_CORE_DATA;
    stall(true, reason, inst);  // Remember to unstall again afterwards.

    if (DEBUG)
      cout << this->name() << " waiting for channel " << (int)channel << endl;

    next_trigger(parent()->receivedDataEvent(channel) | cancelEvent);
  }

  return haveData;
}

bool Decoder::connectionFromMemory(ChannelIndex buffer) const {
  // Convert from input buffer index to input channel index.
  ChannelIndex channel = Registers::fromChannelID(buffer);
  return parent()->connectionFromMemory(channel);
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
  output.memoryOp(PAYLOAD);

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
  if (!inst.predicated())
    return true;

  // Predicated instructions which complete in this pipeline stage and
  // which may access channel data.
  if ((!inst.isExecuteStageOperation() && !inst.isMemoryOperation()) ||
      isFetch(inst.opcode()) ||
      Registers::isChannelEnd(inst.sourceReg1()) ||
      Registers::isChannelEnd(inst.sourceReg2())) {
    short predBits = inst.predicate();
    bool predicate = parent()->predicate(inst);

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
  switch (inst.opcode()) {
    case InstructionMap::OP_PSEL_FETCH:
    case InstructionMap::OP_PSEL_FETCHR:
      return true;
    default: {
      bool irreversible = (!inst.isExecuteStageOperation() && !inst.isMemoryOperation()) ||
                          isFetch(inst.opcode()) ||
                          Registers::isChannelEnd(inst.sourceReg1()) ||
                          Registers::isChannelEnd(inst.sourceReg2());
      return inst.predicated() && irreversible;
    }
  }
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
    stall(true, Instrumentation::Stalls::STALL_FETCH, inst);
    while (!parent()->canFetch()) {
      if (DEBUG)
        cout << this->name() << " waiting to issue " << inst << endl;
      wait(1, sc_core::SC_NS);
    }
    stall(false, Instrumentation::Stalls::STALL_FETCH, inst);
  }

  // Extra forwarding path.
  if (inst.operand1Source() == DecodedInst::BYPASS ||
      inst.operand2Source() == DecodedInst::BYPASS) {
    stall(true, Instrumentation::Stalls::STALL_FORWARDING, inst);
    wait(1.1, sc_core::SC_NS);
    stall(false, Instrumentation::Stalls::STALL_FORWARDING, inst);

    if (inst.operand1Source() == DecodedInst::BYPASS)
      inst.operand1(readRegs(1, inst.sourceReg1()));
    if (inst.operand2Source() == DecodedInst::BYPASS)
      inst.operand2(readRegs(2, inst.sourceReg2()));
  }

  parent()->fetch(inst);
}

void Decoder::stall(bool stall, Instrumentation::Stalls::StallReason reason, const DecodedInst& cause) {
  blocked = stall;
  if (stall)
    Instrumentation::Stalls::stall(id, reason, cause);
  else
    Instrumentation::Stalls::unstall(id, reason, cause);
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

  multiCycleOp = false;
  blocked = false;
  instructionCancelled = false;
  previousInstPredicated = false;
  previousInstSetPredicate = false;

}
