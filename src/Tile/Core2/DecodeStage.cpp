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
    MiddlePipelineStage(name),
    computeHandler("early_compute"),
    readRegHandler1("reg1", REGISTER_PORT_1),
    readRegHandler2("reg2", REGISTER_PORT_2),
    readPredicateHandler("predicate"),
    readCMTHandler("cmt"),
    wocheHandler("credits"),
    selchHandler("buffers") {

  remoteMode = false;

}

void DecodeStage::execute() {
  if (remoteMode) {
    // TODO: set destination
    //      (or create new instruction type for remote execution?)
    // TODO: check for EOP and endRemoteExecution()
  }
  // TODO
  // else if previous instruction writes predicate
  //         and this instruction reads predicate
  //         and modifies state in this stage (reads channel, stalls on selch/woche)
  //   wait for that instruction to update the predicate
  // else if this instruction sends on network and there aren't enough credits
  //   wait for credits
  //   possibly set off register reads, etc. and just block at the end?
  else {

    instruction->readRegisters();
    instruction->readCMT();
    instruction->readPredicate(); // ??
    instruction->earlyCompute();

    previous = instruction;

  }
}

void DecodeStage::computeLatency(opcode_t opcode, function_t fn) {
  // TODO: include a list of operations which are supported here?
  // Helps catch issues with instructions executing at the wrong time, but
  // creates a list which needs to be maintained.
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
  LOKI_LOG(1) << this->name() << " beginning remote execution" << endl;
  remoteMode = true;
  remoteDestination = address;
}

void DecodeStage::endRemoteExecution() {
  LOKI_LOG(1) << this->name() << " ending remote execution" << endl;
  remoteMode = false;
}

void DecodeStage::waitForCredit(ChannelIndex channel) {
  wocheHandler.begin(instruction);
}

void DecodeStage::selectChannelWithData(uint bitmask) {
  core().selectChannelWithData(bitmask);
  selchHandler.begin(instruction);
}

} // end namespace
