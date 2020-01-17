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
    MiddlePipelineStage(name),
    computeHandler("compute"),
    readCMTHandler("readCMT"),
    writeCMTHandler("writeCMT"),
    readCregsHandler("readCregs"),
    writeCregsHandler("writeCregs"),
    readScratchpadHandler("readSpad"),
    writeScratchpadHandler("writeSpad"),
    writePredicateHandler("writePred") {
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
  computeHandler.begin(instruction);
}

void ExecuteStage::readCMT(RegisterIndex index) {
  core().readCMT(index);
  readCMTHandler.begin(instruction);
}

void ExecuteStage::writeCMT(RegisterIndex index, EncodedCMTEntry value) {
  core().writeCMT(index, value);
  writeCMTHandler.begin(instruction);
}

void ExecuteStage::readCreg(RegisterIndex index) {
  core().readCreg(index);
  readCregsHandler.begin(instruction);
}

void ExecuteStage::writeCreg(RegisterIndex index, int32_t value) {
  core().writeCreg(index, value);
  writeCregsHandler.begin(instruction);
}

void ExecuteStage::readScratchpad(RegisterIndex index) {
  core().readScratchpad(index);
  readScratchpadHandler.begin(instruction);
}

void ExecuteStage::writeScratchpad(RegisterIndex index, int32_t value) {
  core().writeScratchpad(index, value);
  writeScratchpadHandler.begin(instruction);
}

void ExecuteStage::writePredicate(bool value) {
  core().writePredicate(value);
  writePredicateHandler.begin(instruction);
}

void ExecuteStage::syscall(int code) {
  core().syscall(code);
  computeHandler.begin(instruction);
  // TODO this requires waiting on a different event to the other use of
  // computeHandler.
}

} // end namespace
