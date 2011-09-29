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
#include "../../../Utility/CoreTrace.h"
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
  CoreTrace::decodeInstruction(id.getPosition(), input.location(), input.endOfIPK());
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
  operation_t operation = input.operation();
  switch(operation) {

    case InstructionMap::LDW :
    case InstructionMap::LDHWU :
    case InstructionMap::LDBU : {
      if(operation == InstructionMap::LDW)
        output.memoryOp(MemoryRequest::LOAD_W);
      else if(operation == InstructionMap::LDHWU)
        output.memoryOp(MemoryRequest::LOAD_HW);
      else
        output.memoryOp(MemoryRequest::LOAD_B);
      break;
    }

    case InstructionMap::STW :
    case InstructionMap::STHW :
    case InstructionMap::STB : {
      multiCycleOp = true;
      output.endOfNetworkPacket(false);
      blockedEvent.notify();

      // The first part of a store is computing the address. This uses the
      // second source register and the immediate.
      output.operation(InstructionMap::ADDUI);
      output.sourceReg1(output.sourceReg2()); output.sourceReg2(0);

      if(operation == InstructionMap::STW)
        output.memoryOp(MemoryRequest::STORE_W);
      else if(operation == InstructionMap::STHW)
        output.memoryOp(MemoryRequest::STORE_HW);
      else
        output.memoryOp(MemoryRequest::STORE_B);
      break;
    }

    case InstructionMap::WOCHE : output.result(output.immediate()); break;
    case InstructionMap::TSTCH : output.result(parent()->testChannel(output.immediate())); break;
    case InstructionMap::SELCH : output.result(parent()->selectChannel()); break;

    case InstructionMap::IBJMP : {
      JumpOffset jump = (JumpOffset)output.immediate();

      // If we know that this jump is going to execute, and the fetch stage has
      // already provided the next instruction, discard the instruction and
      // change the amount we need to jump by.
      if(execute) {
        bool discarded = discardNextInst();
        if(discarded) jump -= BYTES_PER_INSTRUCTION;
        parent()->jump(jump);
      }

      continueToExecute = false;
      break;
    }

    case InstructionMap::RMTEXECUTE : {
      remoteExecute = true;
      sendChannel = output.channelMapEntry();

      if(DEBUG) cout << this->name() << " beginning remote execution" << endl;

      continueToExecute = false;
      break;
    }

    //case InstructionMap::RMTNXIPK :

    case InstructionMap::FETCH:
    case InstructionMap::FETCHPST:
    case InstructionMap::FILL:
    case InstructionMap::PSELFETCH: {
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
    case InstructionMap::IWTR : {
      if(Registers::isChannelEnd(input.destination())) {
        output.destination(readRegs(input.destination()));
        output.operation(InstructionMap::OR);
      }
    }

    default: break;
  } // end switch

  blocked = false;
  blockedEvent.notify();

  return continueToExecute;
}

bool Decoder::ready() const {
  return !multiCycleOp && !blocked;
}

void Decoder::waitForOperands(const DecodedInst& dec) {
  // Wait until all data has arrived from the network.
  if(Registers::isChannelEnd(dec.destination()))
    waitUntilArrival(Registers::toChannelID(dec.destination()));
  if(Registers::isChannelEnd(dec.sourceReg1()))
    waitUntilArrival(Registers::toChannelID(dec.sourceReg1()));
  if(Registers::isChannelEnd(dec.sourceReg2()))
    waitUntilArrival(Registers::toChannelID(dec.sourceReg2()));
}

void Decoder::setOperand1(DecodedInst& dec) {
  RegisterIndex reg = dec.sourceReg1();
  dec.operand1(readRegs(reg, dec.operation() == InstructionMap::IRDR));
}

void Decoder::setOperand2(DecodedInst& dec) {
  if(dec.hasSrcReg2())
    dec.operand2(readRegs(dec.sourceReg2()));
  else
    dec.operand2(dec.immediate());
}

int32_t Decoder::readRegs(RegisterIndex index, bool indirect) {
  return parent()->readReg(index, indirect);
}

void Decoder::waitUntilArrival(ChannelIndex channel) {
  // Test the channel to see if the data is already there.
  if(!parent()->testChannel(channel)) {
    stall(true, Stalls::INPUT);

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
  instruction.operation(InstructionMap::OR);
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
}
