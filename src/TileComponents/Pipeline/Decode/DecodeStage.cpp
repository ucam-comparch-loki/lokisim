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

    bool usefulOutput;

    // If we are in remote execution mode, send this instruction without
    // executing it.
    if (rmtexecuteChannel != Instruction::NO_CHANNEL) {
      decoded = inst;
      remoteExecute(decoded);

      // Drop out of remote execution mode at the end of the packet.
      if (inst.endOfIPK())
        endRemoteExecution();

      usefulOutput = true;
    }
    // Otherwise, pass the instruction through the decoder.
    else {
      readChannelMapTable(inst);
      usefulOutput = decoder.decodeInstruction(inst, decoded);
    }

    // Send the output, if there is any.
    if (usefulOutput) {
      // Stall until we are allowed to send network data.
      if (decoded.sendsOnNetwork())
        waitOnCredits(decoded);

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

void         DecodeStage::readChannelMapTable(DecodedInst& inst) {
  if (previousChannelValid) {
    inst.cmtEntry(previousChannel);
  }
  else {
    MapIndex channel = inst.channelMapEntry();

    if (channel == Instruction::NO_CHANNEL)
      return;

    ChannelMapEntry& cmtEntry = channelMapTableEntry(channel);
    ChannelID destination = cmtEntry.getDestination();

    if (destination.isNullMapping())
      return;

    inst.cmtEntry(cmtEntry.read());
    previousChannel = cmtEntry.read();
  }

  // We will want to reuse the value we read if this is not the last flit in
  // the packet.
  previousChannelValid = !inst.endOfNetworkPacket();
}

// Not const because of the wait().
void DecodeStage::waitOnCredits(DecodedInst& inst) {
  ChannelMapEntry& cmtEntry = channelMapTableEntry(inst.channelMapEntry());
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
}

ChannelMapEntry& DecodeStage::channelMapTableEntry(MapIndex entry) const {
  return core()->channelMapTable[entry];
}

void         DecodeStage::startRemoteExecution(const DecodedInst& inst) {
  // TODO: use the same previousChannel as multi-flit operations
  rmtexecuteChannel = inst.channelMapEntry();
  rmtexecuteCMT = inst.cmtEntry();

  if (DEBUG)
    cout << this->name() << " beginning remote execution" << endl;
}

void         DecodeStage::endRemoteExecution() {
  rmtexecuteChannel = Instruction::NO_CHANNEL;

  if (DEBUG)
    cout << this->name() << " ending remote execution" << endl;
}

void         DecodeStage::remoteExecute(DecodedInst& instruction) const {
  // "Re-encode" the instruction so it can be sent. In practice, it wouldn't
  // have been decoded yet, so there would be no extra work here.
  Instruction encoded = instruction.toInstruction();

  // The data to be sent is the instruction itself.
  instruction.result(encoded.toLong());
  instruction.channelMapEntry(rmtexecuteChannel);
  instruction.cmtEntry(rmtexecuteCMT);

  // Would ideally like network packet to mirror instruction packet, but this
  // can block us from sending a request for the next cache line of the packet.
  instruction.endOfNetworkPacket(true);

  // Prevent other stages from trying to execute this instruction.
  instruction.predicate(Instruction::ALWAYS);
  instruction.opcode(InstructionMap::OP_OR);
  instruction.destination(0);
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

  core()->checkTags(fetchAddress, inst.opcode(), inst.cmtEntry());
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
  rmtexecuteChannel = Instruction::NO_CHANNEL;
  rmtexecuteCMT = 0;

  previousChannel = 0;
  previousChannelValid = false;

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

  SC_METHOD(updateReady);
  sensitive << decoder.stalledEvent();
  // do initialise

}
