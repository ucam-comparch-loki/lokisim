/*
 * DecodeStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "DecodeStage.h"
#include "../../Cluster.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/Instruction.h"

void         DecodeStage::execute() {
  while(true) {
    wait(newInstructionEvent);
    newInput(currentInst);
    instructionCompleted();
  }
}

void         DecodeStage::updateReady() {
  bool ready = !isStalled();

  // Write our current stall status (only if it changed).
  if(ready != readyOut.read()) {
    readyOut.write(ready);

    if(DEBUG && !ready) cout << this->name() << " stalled." << endl;
  }
}

void         DecodeStage::newInput(DecodedInst& inst) {
  if(DEBUG) cout << decoder.name() << " received Instruction: "
                 << inst << endl;

  // If this is the first instruction of a new packet, update the current
  // packet pointer.
  if(startingNewPacket) parent()->updateCurrentPacket(inst.location());

  // The next instruction will be the start of a new packet if this is the
  // end of the current one.
  startingNewPacket = inst.endOfIPK();

  // Use a while loop to decode the instruction in case multiple outputs
  // are produced.
  while(true) {
    DecodedInst decoded;

    // Some instructions don't produce any output, or won't be executed.
    bool usefulOutput = decoder.decodeInstruction(inst, decoded);

    // Send the output, if there is any.
    if(usefulOutput) {
      // Need to access the channel map table inside the loop because decoding
      // the instruction may change its destination.
      readChannelMapTable(decoded);

      // Need to double check whether we are able to send, because we may be
      // sending multiple outputs.
      while(!canSendInstruction()) {
        waitingToSend = true;
        wait(canSendEvent());
      }

      waitingToSend = false;
      outputInstruction(decoded);
    }

    // If the decoder is ready, we have finished the decode.
    if(decoder.ready()) break;
    // Try again next cycle if the decoder is still busy.
    else wait(clock.posedge_event());
  }
}

bool         DecodeStage::isStalled() const {
  return !decoder.ready() && !waitingToSend;
}

int32_t      DecodeStage::readReg(RegisterIndex index, bool indirect) const {
  return parent()->readReg(index, indirect);
}

bool         DecodeStage::predicate() const {
  // true = wait for the execute stage to write the predicate first, if
  // necessary
  return parent()->readPredReg(true);
}

void         DecodeStage::readChannelMapTable(DecodedInst& inst) const {
  MapIndex channel = inst.channelMapEntry();
  if(channel != Instruction::NO_CHANNEL) {
    ChannelID destination = parent()->channelMapTable.read(channel);

    if(!destination.isNullMapping()) {
      inst.networkDestination(destination);
      inst.usesCredits(parent()->channelMapTable.getEntry(channel).usesCredits());
    }
  }
}

const ChannelMapEntry& DecodeStage::channelMapTableEntry(MapIndex entry) const {
  return parent()->channelMapTable.getEntry(entry);
}

int32_t      DecodeStage::readRCET(ChannelIndex index) {
  return rcet.read(index);
}

bool         DecodeStage::testChannel(ChannelIndex index) const {
  return rcet.testChannelEnd(index);
}

ChannelIndex DecodeStage::selectChannel() {
  return rcet.selectChannelEnd();
}

const sc_event& DecodeStage::receivedDataEvent(ChannelIndex buffer) const {
  return rcet.receivedDataEvent(buffer);
}

bool         DecodeStage::inCache(const MemoryAddr addr, opcode_t operation) const {
  return parent()->inCache(addr, operation);
}

bool         DecodeStage::readyToFetch() const {
  return parent()->readyToFetch();
}

void         DecodeStage::jump(JumpOffset offset) const {
  parent()->jump(offset);
}

void         DecodeStage::unstall() {
  decoder.cancelInstruction();

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

  dataIn.init(NUM_RECEIVE_CHANNELS);
  flowControlOut.init(NUM_RECEIVE_CHANNELS);

  // Connect everything up
  rcet.clock(clock);
  for(uint i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    rcet.fromNetwork[i](dataIn[i]);
    rcet.flowControl[i](flowControlOut[i]);
  }

  readyOut.initialize(false);

  SC_THREAD(execute);

  SC_METHOD(updateReady);
  sensitive << decoder.stalledEvent();
  // do initialise

}
