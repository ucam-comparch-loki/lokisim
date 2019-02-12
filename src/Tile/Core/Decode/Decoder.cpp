/*
 * Decoder.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "Decoder.h"

#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/Instruction.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../Core.h"
#include "DecodeStage.h"
#include "../RegisterFile.h"
#include "../../../Utility/Instrumentation.h"
#include "../../../Utility/ISA.h"

typedef RegisterFile Registers;

void Decoder::instructionFinished() {
  // If the instruction will not reach the ALU, record that it executed here.
  if (/*ENERGY_TRACE &&*/ !current.isExecuteStageOperation())
    Instrumentation::executed(parent().core(), current, execute);

  // Do ibjmps here, once we know whether or not this instruction will execute.
  if (current.opcode() == ISA::OP_IBJMP) {
    JumpOffset jump = (JumpOffset)current.immediate();

    // If we know that this jump is going to execute, and the fetch stage has
    // already provided the next instruction, discard the instruction and
    // change the amount we need to jump by.
    if (execute) {
      bool discarded = discardNextInst();
      if(discarded) jump -= BYTES_PER_WORD;
      parent().jump(jump);
    }
  }
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

  if (/*ENERGY_TRACE &&*/ !input.isExecuteStageOperation()) {
    Instrumentation::executed(parent().core(), input, execute);
    parent().instructionExecuted();
  }

  // If we know the instruction won't execute, stop it here.
  if (!execute)
    return false;

  // Wait for any unavailable operands to arrive (either over the network, or
  // by being forwarded from the execute stage).
  waitForOperands(input);

  // Terminate the instruction here if it has been cancelled.
  if (instructionCancelled) {
    LOKI_LOG << this->name() << " aborting " << input << endl;
    instructionCancelled = false;

    return false;
  }

  // Some operations will not need to continue to the execute stage. They
  // complete all their work in the decode stage.
  bool continueToExecute = true;

  // Perform any operation-specific work.
  opcode_t operation = input.opcode();
  switch (operation) {

    case ISA::OP_LDW:
      output.memoryOp(LOAD_W); break;
    case ISA::OP_LDHWU:
      output.memoryOp(LOAD_HW); break;
    case ISA::OP_LDBU:
      output.memoryOp(LOAD_B); break;
    case ISA::OP_LDL:
      output.memoryOp(LOAD_LINKED); break;
    case ISA::OP_STW:
      output.memoryOp(STORE_W); break;
    case ISA::OP_STHW:
      output.memoryOp(STORE_HW); break;
    case ISA::OP_STB:
      output.memoryOp(STORE_B); break;
    case ISA::OP_STC:
      output.memoryOp(STORE_CONDITIONAL); break;
    case ISA::OP_LDADD:
      output.memoryOp(LOAD_AND_ADD); break;
    case ISA::OP_LDOR:
      output.memoryOp(LOAD_AND_OR); break;
    case ISA::OP_LDAND:
      output.memoryOp(LOAD_AND_AND); break;
    case ISA::OP_LDXOR:
      output.memoryOp(LOAD_AND_XOR); break;
    case ISA::OP_EXCHANGE:
      output.memoryOp(EXCHANGE); break;

    case ISA::OP_IRDR:
      multiCycleOp = true;
      blockedEvent.notify();

      // Read register
      setOperand1(output);

      // Wait for result from execute stage, if necessary.
      if (output.operand1Source() == DecodedInst::BYPASS) {
        stall(true, Instrumentation::Stalls::STALL_FORWARDING, output);
        wait(1.1, sc_core::SC_NS);
        stall(false, Instrumentation::Stalls::STALL_FORWARDING, output);
        output.operand1(parent().getForwardedData());
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
    case ISA::OP_LUI: output.sourceReg1(output.destination()); break;

    // Pause execution until the specified channel has at least the given
    // number of credits.
    case ISA::OP_WOCHE: {
      uint targetCredits = output.immediate();
      ChannelMapEntry& cme = parent().channelMapTableEntry(output.channelMapEntry());
      while (!cme.haveNCredits(targetCredits)) {
        stall(true, Instrumentation::Stalls::STALL_OUTPUT, output);
        wait(cme.creditArrivedEvent());
      }
      stall(false, Instrumentation::Stalls::STALL_OUTPUT, output);
      continueToExecute = false;
      break;
    }

    case ISA::OP_TSTCHI:
    case ISA::OP_TSTCHI_P:
      output.result(parent().testChannel(output.immediate())); break;

    case ISA::OP_SELCH:
      output.result(parent().selectChannel(output.immediate(), output)); break;

    case ISA::OP_IBJMP: {
      JumpOffset jump = (JumpOffset)output.immediate();

      // If we know that this jump is going to execute, and the fetch stage has
      // already provided the next instruction, discard the instruction and
      // change the amount we need to jump by.
      if (execute) {
        bool discarded = discardNextInst();
        if (discarded)
          jump--;
        parent().jump(jump);
      }

      continueToExecute = false;
      break;
    }

    case ISA::OP_RMTEXECUTE: {
      parent().startRemoteExecution(output);
      continueToExecute = false;
      break;
    }

    case ISA::OP_RMTNXIPK: {
      Instruction inst = input.toInstruction();
      inst.opcode(ISA::OP_NXIPK);
      output.result(inst.toLong());
      break;
    }

    case ISA::OP_FETCHR:
    case ISA::OP_FETCHPSTR:
    case ISA::OP_FILLR:
    case ISA::OP_PSEL_FETCHR:
    case ISA::OP_FETCH:
    case ISA::OP_FETCHPST:
    case ISA::OP_FILL:
    case ISA::OP_PSEL_FETCH:
      continueToExecute = false;
      break;

    default: break;

  } // end switch

  // Gather the operands this instruction needs.
  setOperand1(output);
  setOperand2(output);

  if (execute && isFetch(output.opcode()))
    fetch(output);

  return continueToExecute;
}

bool Decoder::ready() const {
  return !multiCycleOp && !blocked && outputsRemaining == 0;
}

void Decoder::waitForOperands(const DecodedInst& dec) {
  // Wait until all data has arrived from the network.
  if (ISA::hasDestReg(dec.opcode()) && parent().core().regs.isChannelEnd(dec.destination()))
    waitUntilArrival(parent().core().regs.toChannelID(dec.destination()), dec);
  if (ISA::hasSrcReg1(dec.opcode())) {
    if (parent().core().regs.isChannelEnd(dec.sourceReg1()))
      waitUntilArrival(parent().core().regs.toChannelID(dec.sourceReg1()), dec);
    if (dec.isDecodeStageOperation() && needsForwarding(dec.sourceReg1())) {
      stall(true, Instrumentation::Stalls::STALL_FORWARDING, dec);
      // HACK! May take multiple cycles. FIXME
      // Add an extra 0.1 cycles to ensure that the result is ready for forwarding.
      // Would ideally like to use execute.executedEvent(), but then also need
      // to check for whether the instruction has already executed.
      wait(1.1, sc_core::SC_NS);
      stall(false, Instrumentation::Stalls::STALL_FORWARDING, dec);
    }
  }
  if (ISA::hasSrcReg2(dec.opcode()) && parent().core().regs.isChannelEnd(dec.sourceReg2()))
    waitUntilArrival(parent().core().regs.toChannelID(dec.sourceReg2()), dec);
}

bool Decoder::needsForwarding(RegisterIndex reg) const {
  // Forwarding will be required if there is any instruction between the decode
  // stage and the write back stage which will modify the contents of reg.
  return !parent().core().regs.isReserved(reg)
      && (reg == parent().core().execute.currentInstruction().destination());
}

void Decoder::setOperand1(DecodedInst& dec) {
  RegisterIndex reg = dec.sourceReg1();

  // Read the register in all situations because even if we decide that
  // forwarding is required, we don't yet know whether the instruction will
  // be executed.
  dec.operand1(readRegs(1, reg));

  if (needsForwarding(reg))
    dec.operand1Source(DecodedInst::BYPASS);
  else
    dec.operand1Source(DecodedInst::REGISTER);
}

void Decoder::setOperand2(DecodedInst& dec) {
  if (dec.hasSrcReg2()) {
    RegisterIndex reg = dec.sourceReg2();
    dec.operand2(readRegs(2, reg));

    if (needsForwarding(reg))
      dec.operand2Source(DecodedInst::BYPASS);
    else
      dec.operand2Source(DecodedInst::REGISTER);
  }
  else if (dec.hasImmediate()) {
    dec.operand2(dec.immediate());
    dec.operand2Source(DecodedInst::IMMEDIATE);
  }
}

int32_t Decoder::readRegs(PortIndex port, RegisterIndex index, bool indirect) {
  return parent().readReg(port, index, indirect);
}

void Decoder::waitUntilArrival(ChannelIndex channel, const DecodedInst& inst) {
  if (instructionCancelled)
    return;

  // Test the channel to see if the data is already there.
  if (!parent().testChannel(channel)) {
    bool fromMemory = connectionFromMemory(channel);
    Instrumentation::Stalls::StallReason reason =
        fromMemory ? Instrumentation::Stalls::STALL_MEMORY_DATA
                   : Instrumentation::Stalls::STALL_CORE_DATA;

    stall(true, reason, inst);

    LOKI_LOG << this->name() << " waiting for channel " << (int)channel << endl;

    // Wait until something arrives.
    wait(parent().receivedDataEvent(channel) | cancelEvent);

    stall(false, reason, inst);
  }
}

void Decoder::waitForOperands2(const DecodedInst& inst) {
  haveAllOperands = false;

  // For each of the three possible registers described by the instruction:
  //  1. See if the register corresponds to an input channel.
  //  2. If the channel is empty, wait for data to arrive.
  if (needDestination && ISA::hasDestReg(inst.opcode()) && parent().core().regs.isChannelEnd(inst.destination())) {
    needDestination = false;
    if (!checkChannelInput(parent().core().regs.toChannelID(inst.destination()), inst))
      return;
  }
  if (needOperand1 && ISA::hasSrcReg1(inst.opcode()) && parent().core().regs.isChannelEnd(inst.sourceReg1())) {
    needOperand1 = false;
    if (!checkChannelInput(parent().core().regs.toChannelID(inst.sourceReg1()), inst))
      return;
  }
  if (needOperand2 && ISA::hasSrcReg2(inst.opcode()) && parent().core().regs.isChannelEnd(inst.sourceReg2())) {
    needOperand2 = false;
    if (!checkChannelInput(parent().core().regs.toChannelID(inst.sourceReg2()), inst))
      return;
  }

  haveAllOperands = true;

  // If the decoder blocked waiting for any input, unblock it now that all data
  // have arrived.
  if (blocked) {
    stall(false, Instrumentation::Stalls::STALL_MEMORY_DATA, inst);
    stall(false, Instrumentation::Stalls::STALL_CORE_DATA, inst);
  }
}

bool Decoder::checkChannelInput(ChannelIndex channel, const DecodedInst& inst) {
  bool haveData = parent().testChannel(channel);

  if (!haveData) {
    bool fromMemory = connectionFromMemory(channel);
    Instrumentation::Stalls::StallReason reason =
        fromMemory ? Instrumentation::Stalls::STALL_MEMORY_DATA
                   : Instrumentation::Stalls::STALL_CORE_DATA;
    stall(true, reason, inst);  // Remember to unstall again afterwards.

    LOKI_LOG << this->name() << " waiting for channel " << (int)channel << endl;

    next_trigger(parent().receivedDataEvent(channel) | cancelEvent);
  }

  return haveData;
}

bool Decoder::connectionFromMemory(ChannelIndex buffer) const {
  // Convert from input buffer index to input channel index.
  ChannelIndex channel = parent().core().regs.fromChannelID(buffer);
  return parent().connectionFromMemory(channel);
}

bool Decoder::discardNextInst() const {
  return parent().discardNextInst();
}

void Decoder::cancelInstruction() {
  if (blocked) {
    instructionCancelled = true;
    cancelEvent.notify();
  }
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
  if (inst.isDecodeStageOperation() ||
      parent().core().regs.isChannelEnd(inst.sourceReg1()) ||
      parent().core().regs.isChannelEnd(inst.sourceReg2())) {
    short predBits = inst.predicate();
    bool predicate = parent().predicate(inst);

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
    case ISA::OP_PSEL_FETCH:
    case ISA::OP_PSEL_FETCHR:
      return true;
    default: {
      bool irreversible = inst.isDecodeStageOperation() ||
          parent().core().regs.isChannelEnd(inst.sourceReg1()) ||
          parent().core().regs.isChannelEnd(inst.sourceReg2());
      return inst.predicated() && irreversible;
    }
  }
}

bool Decoder::isFetch(opcode_t opcode) const {
  bool fetch;

  switch (opcode) {
    case ISA::OP_FETCHR:
    case ISA::OP_FETCHPSTR:
    case ISA::OP_FILLR:
    case ISA::OP_PSEL_FETCHR:
    case ISA::OP_FETCH:
    case ISA::OP_FETCHPST:
    case ISA::OP_FILL:
    case ISA::OP_PSEL_FETCH:
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
  if (!parent().canFetch()) {
    stall(true, Instrumentation::Stalls::STALL_FETCH, inst);
    while (!parent().canFetch()) {
      LOKI_LOG << this->name() << " waiting to issue " << inst << endl;
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

  parent().fetch(inst);
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

DecodeStage& Decoder::parent() const {
  return static_cast<DecodeStage&>(*(this->get_parent_object()));
}

void Decoder::reportStalls(ostream& os) {
  if (!ready()) {
    DecodedInst& inst = parent().currentInst;
    os << this->name() << " unable to complete " << LOKI_HEX(inst.location()) << " " << inst << endl;

    if (needOperand1 || (inst.hasOperand1() && parent().core().regs.isChannelEnd(inst.sourceReg1())))
      os << "  Waiting for operand 1." << endl;
    if (needOperand2 || (inst.hasOperand2() && parent().core().regs.isChannelEnd(inst.sourceReg2())))
      os << "  Waiting for operand 2." << endl;
    if (multiCycleOp)
      os << "  Operation takes multiple cycles." << endl;
  }
}

Decoder::Decoder(const sc_module_name& name, const ComponentID& ID) :
    LokiComponent(name, ID) {

  continueToExecute = false;
  execute = false;
  haveAllOperands = true;
  outputsRemaining = 0;
  needDestination = false;
  needOperand1 = false;
  needOperand2 = false;

  multiCycleOp = false;
  blocked = false;
  instructionCancelled = false;

}
