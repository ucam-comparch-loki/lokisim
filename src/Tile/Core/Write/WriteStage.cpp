/*
 * WriteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "WriteStage.h"

#include "../Core.h"
#include "../../../Utility/Arguments.h"
#include "../../../Utility/Instrumentation/Stalls.h"
#include "../../../ISA/ISA.h"

void WriteStage::execute() {
  newInput(currentInst);
//  bool packetInProgress = !currentInst.endOfNetworkPacket();

  if (Arguments::csimTrace())
    core().trace(currentInst);

  instructionCompleted();
}

void WriteStage::newInput(DecodedInst& inst) {
  // Write to registers (they ignore the write if the index is invalid).
  if (ISA::storesResult(inst.opcode()))
    writeReg(inst.destination(), inst.result());
  else if (inst.opcode() == ISA::OP_IWTR)
    writeReg(inst.operand2(), inst.result());
}

void WriteStage::updateReady() {
  bool ready = !isStalled();

  // Write our current stall status.
  if (ready != oReady.read()) {
    oReady.write(ready);

    if (ready)
      Instrumentation::Stalls::unstall(id(), Instrumentation::Stalls::STALL_OUTPUT, currentInst);
    else
      Instrumentation::Stalls::stall(id(), Instrumentation::Stalls::STALL_OUTPUT, currentInst);

    if (!ready)
      LOKI_LOG(3) << this->name() << " stalled." << endl;
  }
}

bool WriteStage::isStalled() const {
  return scet.full();
}

void WriteStage::writeReg(RegisterIndex reg, int32_t value, bool indirect) const {
  core().writeReg(reg, value, indirect);
}

WriteStage::WriteStage(sc_module_name name,
                       const fifo_parameters_t& fifoParams,
                       uint numCores,
                       uint numMemories) :
    LastPipelineStage(name),
    iFetch("iFetch"),
    iData("iData"),
    oReady("oReady"),
    oDataLocal("oDataMulticast"),
    oDataMemory("oDataMemory"),
    scet("scet", fifoParams, numCores, numMemories) {

  // Connect the SCET to the network.
  scet.clock(clock);
  scet.iFetch(iFetch);
  scet.iData(iData);
  oDataLocal(scet.oMulticast);
  oDataMemory(scet.oData);

  SC_METHOD(execute);
  sensitive << newInstructionEvent;
  dont_initialize();

  SC_METHOD(updateReady);
  sensitive << scet.stallChangedEvent();
  // do initialise
}
