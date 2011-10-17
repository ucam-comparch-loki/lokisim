/*
 * Decoder.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "Decoder.h"
#include "DecodeStage.h"
#include "../IndirectRegisterFile.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/Instruction.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../../../Utility/InstructionMap.h"
#include "../../../Utility/Instrumentation/Stalls.h"

typedef IndirectRegisterFile Registers;

bool Decoder::decodeInstruction(const DecodedInst& input, DecodedInst& output) {
  // If this operation produces multiple outputs, produce the next one.
  if(multiCycleOp) return continueOp(input, output);

  // In practice, we wouldn't be receiving a decoded instruction, and the
  // decode would be done here, but it simplifies various interfaces if it is
  // done this way.
  output = input;

  // If we are in remote execution mode, and this instruction is marked, send it.
  if(remoteExecute) {
    if(input.predicate() == Instruction::P) {
      remoteExecution(output);
      return true;
    }
    // Drop out of remote execution mode when we find an unmarked instruction.
    else {
      remoteExecute = false;
      sendChannel = -1;
      if(DEBUG) cout << this->name() << " ending remote execution" << endl;
    }
  }

  blocked = true;
  blockedEvent.notify();

  // Determine if this instruction should execute. This may require waiting for
  // the previous instruction to set the predicate bit if this instruction
  // is completed in the decode stage, or if the instruction may perform an
  // irreversible channel read.
  bool execute = shouldExecute(input);

  Instrumentation::decoded(id, input);
  if(!input.isALUOperation()) Instrumentation::operation(id, input, execute);

  // If the instruction may perform irreversible reads from a channel-end,
  // and we know it won't execute, stop it here.
  if(!execute && (Registers::isChannelEnd(input.sourceReg1()) ||
                  Registers::isChannelEnd(input.sourceReg2()))) {
    blocked = false;
    blockedEvent.notify();
    return true;
  }

  // Wait for any unavailable operands to arrive (either over the network, or
  // by being forwarded from the execute stage).
  waitForOperands(input);

  // Some operations will not need to continue to the execute stage. They
  // complete all their work in the decode stage.
  bool continueToExecute = true;

  // Perform any operation-specific work.
  opcode_t operation = input.opcode();
  switch(operation) {

    case InstructionMap::OP_LDW:
    case InstructionMap::OP_LDHWU:
    case InstructionMap::OP_LDBU: {
      if(operation == InstructionMap::OP_LDW)
        output.memoryOp(MemoryRequest::LOAD_W);
      else if(operation == InstructionMap::OP_LDHWU)
        output.memoryOp(MemoryRequest::LOAD_HW);
      else
        output.memoryOp(MemoryRequest::LOAD_B);
      break;
    }

    case InstructionMap::OP_STW:
    case InstructionMap::OP_STHW:
    case InstructionMap::OP_STB: {
      multiCycleOp = true;
      output.endOfNetworkPacket(false);
      blockedEvent.notify();

      // The first part of a store is computing the address. This uses the
      // second source register and the immediate.
      output.sourceReg1(output.sourceReg2()); output.sourceReg2(0);

      if(operation == InstructionMap::OP_STW)
        output.memoryOp(MemoryRequest::STORE_W);
      else if(operation == InstructionMap::OP_STHW)
        output.memoryOp(MemoryRequest::STORE_HW);
      else
        output.memoryOp(MemoryRequest::STORE_B);
      break;
    }

    // lui only overwrites part of the word, so we need to read the word first.
    // Alternative: have an "lui mode" for register-writing.
    case InstructionMap::OP_LUI: output.sourceReg1(output.destination()); break;

    case InstructionMap::OP_WOCHE: output.result(output.immediate()); break;

    case InstructionMap::OP_TSTCH:
    case InstructionMap::OP_TSTCHI:
    case InstructionMap::OP_TSTCH_P:
    case InstructionMap::OP_TSTCHI_P:
      output.result(parent()->testChannel(output.immediate())); break;

    case InstructionMap::OP_SELCH: output.result(parent()->selectChannel()); break;

    case InstructionMap::OP_IBJMP: {
      JumpOffset jump = (JumpOffset)output.immediate();

      // If we know that this jump is going to execute, and the fetch stage has
      // already provided the next instruction, discard the instruction and
      // change the amount we need to jump by.
      if(execute) {
        bool discarded = discardNextInst();
        if(discarded) jump -= BYTES_PER_WORD;
        parent()->jump(jump);
      }

      continueToExecute = false;
      break;
    }

    case InstructionMap::OP_RMTEXECUTE: {
      remoteExecute = true;
      sendChannel = output.channelMapEntry();

      if(DEBUG) cout << this->name() << " beginning remote execution" << endl;

      continueToExecute = false;
      break;
    }

    case InstructionMap::OP_RMTNXIPK :
      // TODO: implement rmtnxipk
      break;

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

  // Gather the operands this instruction needs.
  setOperand1(output);
  setOperand2(output);

  // Some instructions which complete in the decode stage need to do a little
  // work now that they have their operands.
  switch(operation) {

    // A bit of a hack: if we are indirecting through a channel end, we need to
    // consume the value now so that channel reads are in program order. This
    // means the write is no longer indirect, so change the operation to "or".
    case InstructionMap::OP_IWTR: {
      if(Registers::isChannelEnd(input.sourceReg1())) {
        output.sourceReg1(readRegs(input.sourceReg1()));
        output.opcode(InstructionMap::OP_OR);
      }

      output.destination(output.sourceReg1());
    }

    default: break;
  } // end switch

  blocked = false;
  blockedEvent.notify();

  // Update the destination register so the next instruction knows whether its
  // operand(s) will need to be forwarded to it.
  if(execute && InstructionMap::hasDestReg(input.opcode()) && input.destination() != 0)/* && !input.opcode() == InstructionMap::OP_IWTR)*/
    previousDestination = input.destination();
  else
    previousDestination = -1;

  return continueToExecute;
}

bool Decoder::ready() const {
  return !multiCycleOp && !blocked;
}

void Decoder::waitForOperands(const DecodedInst& dec) {
  // Wait until all data has arrived from the network.
  if(InstructionMap::hasDestReg(dec.opcode()) && Registers::isChannelEnd(dec.destination()))
    waitUntilArrival(Registers::toChannelID(dec.destination()));
  if(InstructionMap::hasSrcReg1(dec.opcode()) && Registers::isChannelEnd(dec.sourceReg1()))
    waitUntilArrival(Registers::toChannelID(dec.sourceReg1()));
  if(InstructionMap::hasSrcReg2(dec.opcode()) && Registers::isChannelEnd(dec.sourceReg2()))
    waitUntilArrival(Registers::toChannelID(dec.sourceReg2()));
}

void Decoder::setOperand1(DecodedInst& dec) {
  RegisterIndex reg = dec.sourceReg1();
  bool indirect = dec.opcode() == InstructionMap::OP_IRDR;

  // FIXME: if this is an indirect read, and the pipeline has recently stalled,
  // it may be possible that the instruction producing the operand we need will
  // be sitting in a pipeline register. We therefore won't get the correct operand.

  if((reg == previousDestination) && !indirect)
    dec.operand1Source(DecodedInst::BYPASS);
  else {
    dec.operand1(readRegs(reg, indirect));
    dec.operand1Source(DecodedInst::REGISTER);
  }
}

void Decoder::setOperand2(DecodedInst& dec) {
  if(dec.hasImmediate()) {
    dec.operand2(dec.immediate());
    dec.operand2Source(DecodedInst::IMMEDIATE);
  }
  else if(dec.hasSrcReg2()) {
    RegisterIndex reg = dec.sourceReg2();

    if(reg == previousDestination)
      dec.operand2Source(DecodedInst::BYPASS);
    else {
      dec.operand2(readRegs(dec.sourceReg2()));
      dec.operand2Source(DecodedInst::REGISTER);
    }
  }
}

int32_t Decoder::readRegs(RegisterIndex index, bool indirect) {
  return parent()->readReg(index, indirect);
}

void Decoder::waitUntilArrival(ChannelIndex channel) {
  // Test the channel to see if the data is already there.
  if(!parent()->testChannel(channel)) {
    stall(true, Stalls::INPUT);

    if(DEBUG)
      cout << this->name() << " waiting for channel " << (int)channel << endl;

    // Wait until something arrives.
    wait(parent()->receivedDataEvent(channel));

    stall(false);
  }
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
  bool result;

  if(inst.usesPredicate() && (!inst.isALUOperation() ||
                              Registers::isChannelEnd(inst.sourceReg1()) ||
                              Registers::isChannelEnd(inst.sourceReg2()))) {

    short predBits = inst.predicate();

    result = (predBits == Instruction::P     &&  parent()->predicate()) ||
             (predBits == Instruction::NOT_P && !parent()->predicate());
  }
  else result = true;

  return result;
}

void Decoder::stall(bool stall, int reason) {
  blocked = stall;
  Instrumentation::stalled(id, stall, reason);
  blockedEvent.notify();
}

const sc_event& Decoder::stalledEvent() const {
  return blockedEvent;
}

DecodeStage* Decoder::parent() const {
  return static_cast<DecodeStage*>(this->get_parent());
}

Decoder::Decoder(const sc_module_name& name, const ComponentID& ID) :
	  Component(name, ID) {
  sendChannel = -1;
  remoteExecute = false;
  previousDestination = -1;
}
