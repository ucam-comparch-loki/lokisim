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
  if (DEBUG)
    cout << this->name() << " received Instruction: " << currentInst << endl;

  newInput(currentInst);
//  bool packetInProgress = !currentInst.endOfNetworkPacket();

  if (CSIM_TRACE)
    core()->trace(currentInst);

  instructionCompleted();
}

void WriteStage::newInput(DecodedInst& inst) {
  // Write to registers (they ignore the write if the index is invalid).
  if (InstructionMap::storesResult(inst.opcode()))
    writeReg(inst.destination(), inst.result());
  else if (inst.opcode() == InstructionMap::OP_IWTR)
    writeReg(inst.operand2(), inst.result());
}

void WriteStage::updateReady() {
  bool ready = !isStalled();

  // Write our current stall status.
  if (ready != oReady.read()) {
    oReady.write(ready);

    if (ready)
      Instrumentation::Stalls::unstall(id, Instrumentation::Stalls::STALL_OUTPUT, currentInst);
    else
      Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_OUTPUT, currentInst);

    if (DEBUG && !ready)
      cout << this->name() << " stalled." << endl;
  }
}

bool WriteStage::isStalled() const {
  return scet.full();
}

void WriteStage::writeReg(RegisterIndex reg, int32_t value, bool indirect) const {
  core()->writeReg(reg, value, indirect);
}

void WriteStage::requestArbitration(ChannelID destination, bool request) {
  core()->requestArbitration(destination, request);
}

bool WriteStage::requestGranted(ChannelID destination) const {
  return core()->requestGranted(destination);
}

WriteStage::WriteStage(sc_module_name name, const ComponentID& ID) :
    PipelineStage(name, ID),
    scet("scet", ID, &(core()->channelMapTable)) {

  // Connect the SCET to the network.
  scet.clock(clock);
  scet.iFetch(iFetch);
  scet.iData(iData);
  scet.oDataLocal(oDataLocal);
  scet.oDataGlobal(oDataGlobal);
  scet.iCredit(iCredit);

  SC_METHOD(execute);
  sensitive << newInstructionEvent;
  dont_initialize();

  SC_METHOD(updateReady);
  sensitive << scet.stallChangedEvent();
  // do initialise
}
