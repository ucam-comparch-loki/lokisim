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
#include "../../../Exceptions/BlockedException.h"
#include "../../../Utility/InstructionMap.h"
#include "../../../Utility/Instrumentation/Stalls.h"

typedef IndirectRegisterFile Registers;

bool Decoder::decodeInstruction(const DecodedInst& input, DecodedInst& output) {

  if(multiCycleOp) {
    return continueOp(input, output);
  }

  // In practice, we wouldn't be receiving a decoded instruction, and the
  // decode would be done here, but it simplifies various interfaces if it is
  // done this way.
  output = input;
  haveStalled = false;

  Instrumentation::decoded(id, output);

  // Instructions that never reach the ALU (e.g. fetch) need to know whether
  // they should execute in this stage.
  bool execute = shouldExecute(input);

  // If we are in remote execution mode, send all marked instructions.
  if(remoteExecute) {
    if(output.predicate() == Instruction::P) {
      Instruction copy = input.toInstruction();

      copy.predicate(Instruction::END_OF_PACKET); // Set it to always execute
      output.result(copy.toLong());
      output.channelMapEntry(sendChannel);

      // Prevent other stages from trying to execute this instruction.
      output.operation(InstructionMap::OR);
      output.destination(0);

      return true;
    }
    // Drop out of remote execution mode when we find an unmarked instruction.
    else {
      remoteExecute = false;
      if(DEBUG) cout << this->name() << " ending remote execution" << endl;
    }
  }

  // Only perform this if we aren't blocked to prevent repeated occurrences.
  if(!blocked && !input.isALUOperation())
    Instrumentation::operation(id, input, execute);

  // If the instruction may perform destructive reads from a channel-end,
  // and we know it won't execute, stop it here.
  if(!execute && input.isALUOperation() &&
                (Registers::isChannelEnd(input.sourceReg1()) ||
                 Registers::isChannelEnd(input.sourceReg2()))) {
    blocked = false;
    return true;
  }

  // Wait until all data has arrived, if any is coming from the network.
  if(Registers::isChannelEnd(input.sourceReg1()))
    waitUntilArrival(Registers::toChannelID(input.sourceReg1()));
  if(Registers::isChannelEnd(input.sourceReg2()))
    waitUntilArrival(Registers::toChannelID(input.sourceReg2()));

  settingPredicate = input.setsPredicate();
  previousDest = (input.destination()==0) ? -1 : input.destination();

  uint8_t operation = output.operation();

  // Deal with all operations which require additional communication with
  // other components.
  switch(operation) {

    case InstructionMap::LDW :
    case InstructionMap::LDHWU :
    case InstructionMap::LDBU : {
      setOperand1ToValue(output, output.sourceReg1());
      setOperand2ToValue(output, 0, output.immediate());
      if(operation == InstructionMap::LDW)
           output.memoryOp(MemoryRequest::LOAD_W);
      else if(operation == InstructionMap::LDHWU)
           output.memoryOp(MemoryRequest::LOAD_HW);
      else output.memoryOp(MemoryRequest::LOAD_B);
      break;
    }

    case InstructionMap::STW :
    case InstructionMap::STHW :
    case InstructionMap::STB : {
      multiCycleOp = true;
    
      // If the store operation will need to read from a channel, wait until
      // the data has arrived. This ensures that the two halves of the store
      // can be sent across the network atomically, and so the possibility of
      // deadlock is eliminated.
//      if(Registers::isChannelEnd(output.sourceReg1()))
//        waitUntilArrival(Registers::toChannelID(output.sourceReg1()));

      setOperand1ToValue(output, output.sourceReg2());
      setOperand2ToValue(output, 0, output.immediate());
      output.operation(InstructionMap::ADDUI);
      output.sourceReg1(output.sourceReg2()); output.sourceReg2(0);

      if(operation == InstructionMap::STW)
           output.memoryOp(MemoryRequest::STORE_W);
      else if(operation == InstructionMap::STHW)
           output.memoryOp(MemoryRequest::STORE_HW);
      else output.memoryOp(MemoryRequest::STORE_B);
      break;
    }

    case InstructionMap::STWADDR :
    case InstructionMap::STBADDR : {
      setOperand1ToValue(output, output.sourceReg1());
      setOperand2ToValue(output, 0, output.immediate());
//      if(operation == InstructionMap::STWADDR)
//           output.memoryOp(MemoryRequest::STADDR);
//      else output.memoryOp(MemoryRequest::STBADDR);
      break;
    }

    case InstructionMap::WOCHE : {
      output.result(output.immediate());
      break;
    }

    case InstructionMap::TSTCH : {
      output.result(parent()->testChannel(output.immediate()));
      break;
    }

    case InstructionMap::SELCH : {
      output.result(parent()->selectChannel());
      break;
    }

    case InstructionMap::IBJMP : {
      if(execute) {
        JumpOffset jump = (JumpOffset)output.immediate();

        // We may need to go back by one more instruction if we have waited a
        // cycle here, because the fetch stage may have already supplied
        // the next instruction.
        if(haveStalled) {
          bool discarded = discardNextInst();
          if(discarded) jump -= BYTES_PER_INSTRUCTION;
        }

        parent()->jump(jump);
      }

      // Nothing useful for the rest of the pipeline to do, so stop this
      // instruction here.
      return false;
    }

    case InstructionMap::RMTEXECUTE : {
      remoteExecute = true;
      sendChannel = output.channelMapEntry();

      if(DEBUG) cout << this->name() << " beginning remote execution" << endl;

      // Nothing useful for the rest of the pipeline to do, so stop this
      // instruction here.
      return false;
    }

    case InstructionMap::RMTFETCH : {
      static string opName = "fetch";
      Instruction copy = input.toInstruction();
      copy.opcode(InstructionMap::opcode(opName));
      copy.predicate(Instruction::END_OF_PACKET);
      output.result(copy.toLong());
      break;
    }

    case InstructionMap::RMTFETCHPST : {
      static string opName = "fetchpst";
      Instruction copy = input.toInstruction();
      copy.opcode(InstructionMap::opcode(opName));
      copy.predicate(Instruction::END_OF_PACKET);
      output.result(copy.toLong());
      break;
    }

    //case InstructionMap::RMTFILL :
    //case InstructionMap::RMTNXIPK :

    // Deprecated: use "setchmap 0 rs" instead.
    case InstructionMap::SETFETCHCH : {
      // See if we should execute first?
      setFetchChannel(output.immediate());
      output.result(0);   // Stop the ALU doing anything by storing a result.
      break;
    }

    case InstructionMap::SETCHMAP : {
      // Don't set the result because this register read may be out of date.
      output.operand1(readRegs(output.sourceReg1()));
      // The rest of the table is in the write stage, so we only deal with
      // entry 0 here. Note that at least two cycles are required between
      // storing the value to a register and reading it here.
      if(output.immediate() == 0) {
        setFetchChannel(output.operand1());
      }
      break;
    }

    case InstructionMap::FETCH :
    case InstructionMap::FETCHPST : {
      // Fetches are unusual in that they never reach the execute stage, where
      // the predicate bit is usually checked. We have to do the check here
      // instead.
      if(execute) {
        fetch(output.immediate() + readRegs(output.sourceReg1()));
        parent()->setPersistent(operation == InstructionMap::FETCHPST);
      }

      // Nothing useful for the rest of the pipeline to do, so stop this
      // instruction here.
      return false;
    }

    case InstructionMap::PSELFETCH : {
      uint32_t selected;
      if(parent()->predicate()) selected = readRegs(output.sourceReg1());
      else                      selected = readRegs(output.sourceReg2());
      fetch(output.immediate() + selected);
      parent()->setPersistent(false);

      // Nothing useful for the rest of the pipeline to do, so stop this
      // instruction here.
      return false;
    }

    case InstructionMap::IRDR : {
      blocked = true;
      output.result(readRegs(input.sourceReg1(), true));
      blocked = false;
      break;
    }

    case InstructionMap::IWTR : {
      // What about data forwarding?
      blocked = true;
      output.destination(readRegs(input.destination()));
      setOperand1(output);
      blocked = false;
      break;
    }

    default : {
      setOperand1(output);
      setOperand2(output);
    }

  } // end switch

  blocked = false;
  return true;

}

bool Decoder::ready() const {
  return !multiCycleOp && !blocked;
}

void Decoder::setOperand1(DecodedInst& dec) {
  setOperand1ToValue(dec, dec.sourceReg1());
}

void Decoder::setOperand2(DecodedInst& dec) {
  setOperand2ToValue(dec, dec.sourceReg2(), dec.immediate());
}

/* Determine where to read the first operand from: RCET or registers */
void Decoder::setOperand1ToValue(DecodedInst& dec, RegisterIndex reg) {
  if(Registers::isChannelEnd(reg))
    dec.operand1(readRCET(Registers::toChannelID(reg)));
  else
    dec.operand1(readRegs(reg));
}

/* Determine where to get second operand from: immediate, RCET or regs */
void Decoder::setOperand2ToValue(DecodedInst& dec, RegisterIndex reg, int32_t immed) {
  if(dec.hasImmediate()) {
    dec.operand2(immed);
    // This would require use of the sign-extender.
    // Remember to include the energy use.
  }
  else if(Registers::isChannelEnd(reg)) {
    dec.operand2(readRCET(Registers::toChannelID(reg)));
  }
  else {
    dec.operand2(readRegs(reg));
  }
}

int32_t Decoder::readRegs(RegisterIndex index, bool indirect) {
  return parent()->readReg(index, indirect);
}

int32_t Decoder::readRCET(ChannelIndex index) {
  int32_t result = parent()->readRCET(index);
  return result;
}

void Decoder::waitUntilArrival(ChannelIndex channel) {
  // Test the channel to see if the data is already there.
  if(!parent()->testChannel(channel)) {
    blocked = true;
    Instrumentation::stalled(id, true, Stalls::INPUT);

    // Wait until something arrives.
    wait(parent()->receivedDataEvent(channel));
    haveStalled = true;

    blocked = false;
    Instrumentation::stalled(id, false);
  }
}

void Decoder::fetch(MemoryAddr addr) const {
  parent()->fetch(addr);
}

void Decoder::setFetchChannel(uint32_t encodedChannel) const {
	// Encoded channel format:
	// | Tile : 12 | Position : 8 | Channel : 4 | Memory group bits : 4 | Memory line bits : 4 |

	uint newTile = encodedChannel >> 20;
	uint newPosition = (encodedChannel >> 12) & 0xFFUL;
	uint newChannel = (encodedChannel >> 8) & 0xFUL;
	uint newGroupBits = (encodedChannel >> 4) & 0xFUL;
	uint newLineBits = encodedChannel & 0xFUL;

	ChannelID channel(newTile, newPosition, newChannel);

	parent()->setFetchChannel(channel, newGroupBits, newLineBits);
}

bool Decoder::discardNextInst() const {
  return parent()->discardNextInst();
}

/* Sends the second part of a two-flit store operation (the data to send). */
bool Decoder::continueOp(const DecodedInst& input, DecodedInst& output) {
  output = input;

  setOperand1ToValue(output, input.sourceReg1());
  output.memoryOp(MemoryRequest::PAYLOAD_ONLY);

  multiCycleOp = false;
  blocked = false;

  return true;
}

/* Determine whether this instruction should be executed. */
bool Decoder::shouldExecute(const DecodedInst& inst) {

  if((inst.sourceReg1()==previousDest || inst.sourceReg2()==previousDest) &&
     (inst.operation()==InstructionMap::FETCH || inst.operation()==InstructionMap::PSELFETCH)) {
    // We need a value in this cycle which has not yet been computed, so can't
    // be forwarded. We need to wait for one cycle.
    if(DEBUG) cout << this->name()
        << " waiting one cycle for value to be computed." << endl;

    blocked = true;
    Instrumentation::stalled(id, true, Stalls::PREDICATE);
    wait(1, sc_core::SC_NS);
    blocked = false;
    Instrumentation::stalled(id, false);

    haveStalled = true;

  }

  // If the previous instruction sets the predicate bit, and we need to access
  // it in this pipeline stage, we must stall for a cycle until it has been
  // written.
  if(settingPredicate && inst.usesPredicate() && !haveStalled) {

    // We need to know the predicate value in this cycle if we are doing
    // something like a fetch, which is carried out in the decode stage, or if
    // we read from a channel-end, as these reads are destructive.
    if(!inst.isALUOperation() ||
       Registers::isChannelEnd(inst.sourceReg1()) ||
       Registers::isChannelEnd(inst.sourceReg2())) {
      if(DEBUG) cout << this->name()
          << " waiting one cycle for predicate to be set." << endl;

      blocked = true;
      Instrumentation::stalled(id, true, Stalls::PREDICATE);
      wait(1, sc_core::SC_NS);
      blocked = false;
      Instrumentation::stalled(id, false);

      haveStalled = true;
    }
  }

  short predBits = inst.predicate();

  bool result = (predBits == Instruction::ALWAYS) ||
                (predBits == Instruction::END_OF_PACKET) ||
                (predBits == Instruction::P     &&  parent()->predicate()) ||
                (predBits == Instruction::NOT_P && !parent()->predicate());

  return result;

}

DecodeStage* Decoder::parent() const {
  return static_cast<DecodeStage*>(this->get_parent());
}

Decoder::Decoder(sc_module_name name, const ComponentID& ID) : Component(name, ID) {

  sendChannel = -1;
  remoteExecute = false;
  previousDest = -1;

}
