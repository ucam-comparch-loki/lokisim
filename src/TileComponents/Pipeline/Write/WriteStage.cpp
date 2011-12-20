/*
 * WriteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "WriteStage.h"
#include "../../Cluster.h"
#include "../../../Utility/InstructionMap.h"
#include "../../../Utility/Instrumentation/Stalls.h"

const DecodedInst& WriteStage::currentInstruction() const {
  return currentInst;
}

void WriteStage::execute() {
  DecodedInst instruction = instructionIn.read();
  newInput(instruction);
  bool packetInProgress = !instruction.endOfNetworkPacket();

  if(idle.read() == packetInProgress)
    idle.write(!packetInProgress);
}

void WriteStage::newInput(DecodedInst& inst) {
  currentInst = inst;

  // We can't forward data if this is an indirect write because we don't
  // yet know where the data will be written.
  bool indirect = (inst.opcode() == InstructionMap::OP_IWTR);
  if(indirect) currentInst.destination(0);

  // Write to registers (they ignore the write if the index is invalid).
  if(InstructionMap::storesResult(inst.opcode()) || indirect)
    writeReg(inst.destination(), inst.result(), indirect);
}

void WriteStage::reset() {
  currentInst.destination(0);
}

void WriteStage::sendData() {
  scet.write(dataIn.read());
}

void WriteStage::updateReady() {
  bool ready = !isStalled();

  if(DEBUG && !ready && readyOut.read())
    cout << this->name() << " stalled." << endl;

  // Write our current stall status.
  if(ready != readyOut.read()) {
    readyOut.write(ready);
    Instrumentation::stalled(id, !ready, Stalls::STALL_OUTPUT);
  }

  // Wait until some point late in the cycle, so we know that any operations
  // will have completed.
  next_trigger(scet.stallChangedEvent());
}

bool WriteStage::isStalled() const {
  return scet.full();
}

void WriteStage::writeReg(RegisterIndex reg, int32_t value, bool indirect) const {
  parent()->writeReg(reg, value, indirect);
}

const sc_event& WriteStage::requestArbitration(ChannelID destination, bool request) {
  return parent()->requestArbitration(destination, request);
}

bool WriteStage::readyToFetch() const {
  return parent()->readyToFetch();
}

WriteStage::WriteStage(sc_module_name name, const ComponentID& ID) :
    PipelineStage(name, ID),
    scet("scet", ID, &(parent()->channelMapTable)) {

  static const unsigned int NUM_BUFFERS = CORE_OUTPUT_PORTS;

  output      = new loki_out<AddressedWord>[NUM_BUFFERS];
  creditsIn   = new loki_in<AddressedWord>[NUM_BUFFERS];

  // Connect the SCET to the network.
  scet.clock(clock);

  for(unsigned int i=0; i<NUM_BUFFERS; i++) {
    scet.output[i](output[i]);
    scet.creditsIn[i](creditsIn[i]);
  }

  SC_METHOD(execute);
  sensitive << instructionIn;
  dont_initialize();

  SC_METHOD(reset);
  sensitive << clock.pos();
  dont_initialize();

  SC_METHOD(sendData);
  sensitive << dataIn;
  dont_initialize();
}

WriteStage::~WriteStage() {
  delete[] output;
  delete[] creditsIn;
}
