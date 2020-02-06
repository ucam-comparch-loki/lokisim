/*
 * ExecuteStage.cpp
 *
 *  Created on: Aug 14, 2019
 *      Author: db434
 */

#include "ExecuteStage.h"
#include "Core.h"

namespace Compute {

ExecuteStage::ExecuteStage(sc_module_name name) :
    MiddlePipelineStage(name) {
  // Nothing
}

void ExecuteStage::execute() {
  instruction->compute();
  instruction->readCregs();
  instruction->readScratchpad();
  instruction->writePredicate();
  instruction->writeCMT();
  instruction->writeCregs();
  instruction->writeScratchpad();
}

void ExecuteStage::computeLatency(opcode_t opcode, function_t fn) {
  switch (opcode) {
    case ISA::OP_MULLW:
    case ISA::OP_MULHW:
    case ISA::OP_MULHWU:
      // TODO: 2 cycle wait
      break;
    default:
      // TODO: 1 cycle wait
      break;
  }
}

void ExecuteStage::readCMT(RegisterIndex index) {
  // Structural hazard: use port 1 so we have priority over decode stage.
  core().readCMT(instruction, index, 0);
}

void ExecuteStage::writeCMT(RegisterIndex index, EncodedCMTEntry value) {
  core().writeCMT(instruction, index, value);
}

void ExecuteStage::readCreg(RegisterIndex index) {
  core().readCreg(instruction, index);
}

void ExecuteStage::writeCreg(RegisterIndex index, int32_t value) {
  core().writeCreg(instruction, index, value);
}

void ExecuteStage::readScratchpad(RegisterIndex index) {
  core().readScratchpad(instruction, index);
}

void ExecuteStage::writeScratchpad(RegisterIndex index, int32_t value) {
  core().writeScratchpad(instruction, index, value);
}

void ExecuteStage::writePredicate(bool value) {
  core().writePredicate(instruction, value);
}

void ExecuteStage::syscall(int code) {
  core().syscall(instruction, code);
}

} // end namespace
