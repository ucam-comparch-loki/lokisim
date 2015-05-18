/*
 * DecodeStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "DecodeStage.h"
#include "../../Core.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/Instruction.h"
#include "../../../Exceptions/InvalidOptionException.h"
#include "../../../Utility/Trace/LBTTrace.h"

void         DecodeStage::execute() {
  while (true) {
    // The decode stage is the arbiter of whether the pipeline is idle. This
    // is the only stage through which all instructions pass.

    // Only consider the core idle if the next pipeline stage is ready to
    // receive an instruction, but we don't have one to pass to it.
    if (canSendInstruction())
      core()->idle(true);

    wait(newInstructionEvent);
    core()->idle(false);

    newInput(currentInst);
    instructionCompleted();

    // If the instruction is to be executed repeatedly, switch into persistent
    // mode and keep issuing the now-decoded instruction.
    // TODO: make sure currentInst is set correctly
    if (currentInst.persistent())
      persistentInstruction(currentInst);

    wait(clock.posedge_event());
  }
}

void         DecodeStage::updateReady() {
  bool ready = !isStalled();

  // Write our current stall status (only if it changed).
  if (ready != oReady.read()) {
    oReady.write(ready);

    if (DEBUG && !ready)
      cout << this->name() << " stalled." << endl;
  }
}

void         DecodeStage::persistentInstruction(DecodedInst& inst) {
  // TODO: figure out what to do with store instructions.
  // They split into two sub-ops, so we may still need the decoder.

  // Determine if any of the registers read by this instruction are in fact
  // constants. Constants only need to be read once, not every cycle.
  bool constantReg1 = inst.hasSrcReg1() && (inst.sourceReg1() != inst.destination())
      && !RegisterFile::isChannelEnd(inst.sourceReg1());
  bool constantReg2 = inst.hasSrcReg2() && (inst.sourceReg2() != inst.destination())
      && !RegisterFile::isChannelEnd(inst.sourceReg2());

  while (true) {
    wait(clock.posedge_event());
    decoder.waitForOperands(inst);

    // Bail before attempting to read any channel ends if the instruction was
    // cancelled.
    if (decoder.instructionCancelled) {
      decoder.instructionCancelled = false;
      break;
    }

    if (!constantReg1)
      decoder.setOperand1(inst);
    if (!constantReg2)
      decoder.setOperand2(inst);

    // Should do this check earlier, before gathering operands.
    if (!canSendInstruction()) {
      waitingToSend = true;
      wait(canSendEvent());
    }

    waitingToSend = false;
    outputInstruction(inst);

    instructionCompleted();
  }

}

/*
void DecodeStage::execute2() {
  // No matter which state we are in, we must stall if there is no room in the
  // output pipeline register.
  if (!canSendInstruction()) {
    waitingToSend = true;
    next_trigger(canSendEvent());
    return;
  }
  else
    waitingToSend = false;

  // In an attempt to speed up simulation, we split decoding into several
  // sections, each of which could require the pipeline to stall.
  // This avoids the need for expensive SC_THREADs, and reduces the amount of
  // work which must be repeated after stalling.
  switch (state) {

    case INIT:
      state = NEW_INSTRUCTION;
      next_trigger(newInstructionEvent);
      break;

    // All the one-off computation required when a new instruction arrives.
    case NEW_INSTRUCTION:
      if (DEBUG) cout << decoder.name() << " received Instruction: "
                      << currentInst << endl;

      if (startingNewPacket)
        parent()->updateCurrentPacket(currentInst.location());
      startingNewPacket = currentInst.endOfIPK();

      decoder.initialise(currentInst);
      state = DECODE;
      // fall through to next case

    // Some operations (e.g. stores) produce more than one output. Loop until
    // all output has been sent to the next pipeline stage.
    case DECODE:
      decoder.decode();
      state = WAIT_FOR_OPERANDS;
      // fall through to next case

    // Loop while we wait for all operands to arrive. (Some may be coming from
    // the network.) This could also be used if there are not enough ports on
    // the register file, and we need to wait to read another value.
    case WAIT_FOR_OPERANDS:
      decoder.collectOperands();

      // If we do not have all operands, next_trigger will have been set so
      // that this method is called again when an operand arrives.
      if (!decoder.allOperandsReady())
        break;
      else
        state = SEND_RESULT;
        // fall through to next case

    case SEND_RESULT:
      if (decoder.hasOutput()) {
        DecodedInst output = decoder.getOutput();
        readChannelMapTable(output);
        outputInstruction(output);

        // If the decoder is ready, it has produced all output, and is
        // waiting for the next instruction. Otherwise, there is more output,
        // so wait until the next cycle.
        if (decoder.ready()) {
          instructionCompleted();
          decoder.instructionFinished();
          state = NEW_INSTRUCTION;
          next_trigger(newInstructionEvent);
        }
        else {
          state = DECODE;
          next_trigger(clock.posedge_event());
        }
      }
      else {
        instructionCompleted();
        decoder.instructionFinished();
        state = NEW_INSTRUCTION;
        next_trigger(newInstructionEvent);
      }

      break;

    // There are no other states.
    default:
      assert(false);
      break;

  } // end switch
}
*/

void         DecodeStage::newInput(DecodedInst& inst) {
  if (DEBUG) cout << decoder.name() << " received Instruction: "
                  << inst << endl;

  // If this is the first instruction of a new packet, update the current
  // packet pointer.
  if (startingNewPacket)
    core()->updateCurrentPacket(inst.location());

  // The next instruction will be the start of a new packet if this is the
  // end of the current one.
  startingNewPacket = inst.endOfIPK();

  DecodedInst decoded;

  // Use a while loop to decode the instruction in case multiple outputs
  // are produced.
  while (true) {

    // Our register file isn't event-driven, so we need to ensure that data is
    // read from it after the writes have completed.
    wait(sc_core::SC_ZERO_TIME);

    // Some instructions don't produce any output, or won't be executed.
    bool usefulOutput = decoder.decodeInstruction(inst, decoded);

    // Send the output, if there is any.
    if (usefulOutput) {
      // Need to access the channel map table inside the loop because decoding
      // the instruction may change its destination.
      readChannelMapTable(decoded);

      // Need to double check whether we are able to send, because we may be
      // sending multiple outputs.
      while (!canSendInstruction()) {
        waitingToSend = true;
        wait(canSendEvent());
      }

      waitingToSend = false;
      outputInstruction(decoded);
    }

    // If the decoder is ready, we have finished the decode.
    if (decoder.ready()) break;
    // Try again next cycle if the decoder is still busy.
    else wait(clock.posedge_event());
  }

  currentInst = decoded;
}

bool         DecodeStage::isStalled() const {
  return !decoder.ready() && !waitingToSend;
}

int32_t      DecodeStage::readReg(PortIndex port, RegisterIndex index, bool indirect) const {
  return core()->readReg(port, index, indirect);
}

int32_t      DecodeStage::getForwardedData() const {
  return core()->getForwardedData();
}

bool         DecodeStage::predicate(const DecodedInst& inst) const {
  // true = wait for the execute stage to write the predicate first, if
  // necessary
  return core()->readPredReg(true, inst);
}

// Not const because of the wait().
void         DecodeStage::readChannelMapTable(DecodedInst& inst) {
  MapIndex channel = inst.channelMapEntry();

  if (channel == Instruction::NO_CHANNEL)
    return;

  ChannelMapEntry& cmtEntry = channelMapTableEntry(channel);
  ChannelID destination = cmtEntry.getDestination();

  if (destination.isNullMapping())
    return;

  if (!cmtEntry.canSend()) {
    if (DEBUG)
      cout << this->name() << " stalled waiting for credits from " << destination << endl;
    Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_OUTPUT, inst);
    wait(cmtEntry.creditArrivedEvent());
    Instrumentation::Stalls::unstall(id, Instrumentation::Stalls::STALL_OUTPUT, inst);
  }
  cmtEntry.removeCredit();

  if (!iOutputBufferReady.read()) {
    if (DEBUG)
      cout << this->name() << " stalled waiting for output buffer space" << endl;
    Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_OUTPUT, inst);
    wait(iOutputBufferReady.posedge_event());
    Instrumentation::Stalls::unstall(id, Instrumentation::Stalls::STALL_OUTPUT, inst);
  }

  inst.networkDestination(destination);
  inst.usesCredits(cmtEntry.usesCredits());
}

ChannelMapEntry& DecodeStage::channelMapTableEntry(MapIndex entry) const {
  return core()->channelMapTable[entry];
}

int32_t      DecodeStage::readRCET(ChannelIndex index) {
  return rcet.read(index);
}

int32_t      DecodeStage::readRCETDebug(ChannelIndex index) const {
  return rcet.readDebug(index);
}

void DecodeStage::fetch(const DecodedInst& inst) {
  MemoryAddr fetchAddress;

  // Compute the address to fetch from, depending on which operation this is.
  switch (inst.opcode()) {
    case InstructionMap::OP_FETCH:
    case InstructionMap::OP_FETCHPST:
    case InstructionMap::OP_FILL:
      fetchAddress = inst.operand1() + inst.operand2();
      break;

    case InstructionMap::OP_FETCHR:
    case InstructionMap::OP_FETCHPSTR:
    case InstructionMap::OP_FILLR:
      fetchAddress = readReg(1, 1) + BYTES_PER_WORD*inst.operand2();
      break;

    case InstructionMap::OP_PSEL_FETCH:
      fetchAddress = core()->readPredReg(true, inst) ? inst.operand1() : inst.operand2();
      break;

    case InstructionMap::OP_PSEL_FETCHR: {
      int immed1 = inst.immediate();
      int immed2 = inst.immediate2();
      fetchAddress = readReg(1, 1) + BYTES_PER_WORD*(core()->readPredReg(true, inst) ? immed1 : immed2);
      break;
    }

    default:
      throw InvalidOptionException("fetch instruction opcode", inst.opcode());
      break;
  }

  if (Arguments::lbtTrace())
    LBTTrace::setInstructionMemoryAddress(inst.isid(), fetchAddress);

  if (DEBUG)
    cout << this->name() << " fetching from address " << fetchAddress << endl;

  // Tweak the network address based on the memory address to ensure that the
  // correct bank is accessed if the fetch request is sent.
  uint increment = channelMapTableEntry(0).computeAddressIncrement(fetchAddress);
  ChannelID destination = channelMapTableEntry(0).getDestination();
  destination.addPosition(increment);
  ChannelIndex returnTo = channelMapTableEntry(0).getReturnChannel();

  core()->checkTags(fetchAddress, inst.opcode(), destination, returnTo);
}

bool         DecodeStage::canFetch() const {
  return core()->canCheckTags();
}

bool         DecodeStage::connectionFromMemory(ChannelIndex channel) const {
  return core()->channelMapTable.connectionFromMemory(channel);
}

bool         DecodeStage::testChannel(ChannelIndex index) const {
  return rcet.testChannelEnd(index);
}

ChannelIndex DecodeStage::selectChannel(unsigned int bitmask, const DecodedInst& inst) {
  return rcet.selectChannelEnd(bitmask, inst);
}

const sc_event& DecodeStage::receivedDataEvent(ChannelIndex buffer) const {
  return rcet.receivedDataEvent(buffer);
}

void         DecodeStage::jump(JumpOffset offset) const {
  core()->jump(offset);
}

void         DecodeStage::instructionExecuted() {
  core()->cregs.instructionExecuted();
}

void         DecodeStage::unstall() {
  decoder.cancelInstruction();
  currentInst.persistent(false);

  // We are aborting the current instruction packet, so the next instruction
  // will be the start of a new packet.
  startingNewPacket = true;

  // There is the possibility that we are stalled creating the second half
  // of a store message. We will need to send some result so that memory is not
  // stalled forever.
}

DecodeStage::DecodeStage(sc_module_name name, const ComponentID& ID) :
    PipelineStage(name, ID),
    rcet("rcet", ID),
    decoder("decoder", ID) {

  startingNewPacket = true;
  waitingToSend = false;

  iData.init(NUM_RECEIVE_CHANNELS);
  oFlowControl.init(NUM_RECEIVE_CHANNELS);
  oDataConsumed.init(NUM_RECEIVE_CHANNELS);

  // Connect everything up
  rcet.clock(clock);
  for (uint i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    rcet.iData[i](iData[i]);
    rcet.oFlowControl[i](oFlowControl[i]);
    rcet.oDataConsumed[i](oDataConsumed[i]);
  }

  oReady.initialize(false);

  SC_THREAD(execute);
  // TODO - eliminate SC_THREADs
  state = INIT;
//  SC_METHOD(execute2);

  SC_METHOD(updateReady);
  sensitive << decoder.stalledEvent();
  // do initialise

}
