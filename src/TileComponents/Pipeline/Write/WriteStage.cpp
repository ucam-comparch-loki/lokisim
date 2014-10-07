/*
 * WriteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "WriteStage.h"
#include "../../Core.h"
#include "../../../Utility/InstructionMap.h"
#include "../../../Utility/Instrumentation/Stalls.h"

const DecodedInst& WriteStage::currentInstruction() const {
  return currentInst;
}

void WriteStage::execute() {
  newInput(currentInst);
//  bool packetInProgress = !currentInst.endOfNetworkPacket();

  if (CSIM_TRACE)
    parent()->trace(currentInst);

  instructionCompleted();
}

void WriteStage::newInput(DecodedInst& inst) {
  bool indirect = (inst.opcode() == InstructionMap::OP_IWTR);

  // Write to registers (they ignore the write if the index is invalid).
  //if(InstructionMap::storesResult(inst.opcode()) || indirect)
  //  writeReg(inst.destination(), inst.result(), indirect);
  if (InstructionMap::storesResult(inst.opcode()) && !indirect)
    writeReg(inst.destination(), inst.result());

  // We can't forward data if this is an indirect write because we don't
  // know where the data will be written.
  if (indirect)
    inst.destination(0);
}

void WriteStage::sendData() {
  scet.write(iData.read());
}

void WriteStage::updateReady() {
  bool ready = !isStalled();

  // Write our current stall status.
  if (ready != oReady.read()) {
    oReady.write(ready);

    if (ready)
      Instrumentation::Stalls::unstall(id, Instrumentation::Stalls::STALL_OUTPUT);
    else
      Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_OUTPUT);

    if (DEBUG && !ready)
      cout << this->name() << " stalled." << endl;
  }
}

bool WriteStage::isStalled() const {
  return scet.full();
}

void WriteStage::writeReg(RegisterIndex reg, int32_t value, bool indirect) const {
  parent()->writeReg(reg, value, indirect);
}

void WriteStage::requestArbitration(ChannelID destination, bool request) {
  parent()->requestArbitration(destination, request);
}

bool WriteStage::requestGranted(ChannelID destination) const {
  return parent()->requestGranted(destination);
}

bool WriteStage::readyToFetch() const {
  return parent()->readyToFetch();
}

WriteStage::WriteStage(sc_module_name name, const ComponentID& ID) :
    PipelineStage(name, ID),
    scet("scet", ID, &(parent()->channelMapTable)) {

  // Connect the SCET to the network.
  scet.clock(clock);
  scet.oDataLocal(oDataLocal);
  scet.oDataGlobal(oDataGlobal);
  scet.iCredit(iCredit);

  SC_METHOD(execute);
  sensitive << newInstructionEvent;
  dont_initialize();

  SC_METHOD(sendData);
  sensitive << iData;
  dont_initialize();

  SC_METHOD(updateReady);
  sensitive << scet.stallChangedEvent();
  // do initialise
}
