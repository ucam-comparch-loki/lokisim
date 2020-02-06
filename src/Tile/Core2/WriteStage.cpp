/*
 * WriteStage.cpp
 *
 *  Created on: Aug 14, 2019
 *      Author: db434
 */

#include "WriteStage.h"
#include "Core.h"

namespace Compute {

WriteStage::WriteStage(sc_module_name name) :
    LastPipelineStage(name) {
  // Nothing
}

void WriteStage::execute() {
  // TODO: if register access waits for the clock edge, should probably write
  // registers in the execute stage.
  instruction->writeRegisters();
  instruction->sendNetworkData();
}

void WriteStage::writeRegister(RegisterIndex index, int32_t value) {
  core().writeRegister(instruction, index, value);
}

void WriteStage::sendOnNetwork(NetworkData flit) {
  core().sendOnNetwork(instruction, flit);
}

} // end namespace
