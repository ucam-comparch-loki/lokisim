/*
 * DecodeStage.cpp
 *
 *  Created on: Aug 14, 2019
 *      Author: db434
 */

#include "DecodeStage.h"
#include "Core.h"

namespace Compute {

DecodeStage::DecodeStage(sc_module_name name) :
    PipelineStage(name),
    computeHandler("early_compute"),
    readRegHandler1("reg1", REGISTER_PORT_1),
    readRegHandler2("reg2", REGISTER_PORT_2),
    readPredicateHandler("predicate"),
    readCMTHandler("cmt"),
    wocheHandler("credits"),
    selchHandler("buffers") {
  // Nothing
}

void DecodeStage::execute() {
  // TODO Check whether this instruction needs to be executed.

  instruction->readRegisters();
  instruction->readCMT();
  instruction->readPredicate();
  instruction->earlyCompute();
}

void DecodeStage::computeLatency(opcode_t opcode, function_t fn) {
  // TODO: include a list of operations which are supported here?
  switch (opcode) {
    default:
      // TODO: 1 cycle wait
      break;
  }
  computeHandler.begin(instruction);
}

void DecodeStage::readRegister(RegisterIndex index, RegisterPort port) {
  core().readRegister(index, port);
  switch (port) {
    case REGISTER_PORT_1: readRegHandler1.begin(instruction); break;
    case REGISTER_PORT_2: readRegHandler2.begin(instruction); break;
  }
}

void DecodeStage::readPredicate() {
  core().readPredicate();
  readPredicateHandler.begin(instruction);
}

void DecodeStage::readCMT(RegisterIndex index) {
  core().readCMT(index);
  readCMTHandler.begin(instruction);
}

void DecodeStage::fetch(MemoryAddr address,
                        ChannelMapEntry::MemoryChannel channel,
                        bool execute, bool persistent) {
  core().fetch(address, channel, execute, persistent);
}

void DecodeStage::jump(JumpOffset offset) {
  core().jump(offset);
}

void DecodeStage::startRemoteExecution(ChannelID address) {
  core().startRemoteExecution(address);
  // TODO: should we do something here instead?
}

void DecodeStage::waitForCredit(ChannelIndex channel) {
  wocheHandler.begin(instruction);
}

void DecodeStage::selectChannelWithData(uint bitmask) {
  core().selectChannelWithData(bitmask);
  selchHandler.begin(instruction);
}

} // end namespace
