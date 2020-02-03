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
    readCMTHandler("cmt", REGISTER_PORT_2), // Port 2: EXECUTE has read priority
    wocheHandler("credits"),
    selchHandler("buffers") {

  remoteMode = false;

}

// Verilog approach:
//  * ALWAYS stall if the previous instruction updates predicate and this
//    instruction is predicated (even if the instruction does nothing in this
//    stage)
//  * Other stalls (selch/woche/channel read/...) use the old predicate value
//    and rely on the above condition blocking any dangerous state change
//
// Stall reasons:
//  * Waiting for predicate
//  * Reading empty FIFO
//  * Sending to local core, but flow control signals show no FIFO space
//  * Sending to remote core, but no credits
//  * Structural hazard at CMT (previous inst was getchmap, this inst does normal read)
//  * Selch, woche
//
// Of these, we only need to handle the predicate here. Everything else is
// handled when accessing the respective modules.
void DecodeStage::execute() {
  if (remoteMode) {
    bool eop = instruction->isEndOfPacket();

    // TODO: set destination
    //      (or create new instruction type for remote execution?)

    if (eop)
      endRemoteExecution();

    previousInstruction.reset();
  }
  // Stall a cycle if we need a predicate that the previous instruction has yet
  // to compute.
  else if (instruction->isPredicated() && previousInstruction
        && previousInstruction->writesPredicate()
        && !previousInstruction->completedPhase(InstructionInterface::INST_PRED_WRITE)) {
    next_trigger(clock.posedge_event());
  }
  else {

    instruction->readPredicate();
    instruction->readRegisters();
    instruction->readCMT();
    instruction->earlyCompute();

    previousInstruction = instruction;

  }
}

void DecodeStage::computeLatency(opcode_t opcode, function_t fn) {
  switch (opcode) {
    default:
      // TODO: 1 cycle wait
      break;
  }
  computeHandler.begin(instruction);
}

void DecodeStage::readRegister(RegisterIndex index, RegisterPort port) {
  // TODO: Create a separate ForwardingNetwork?
  // All instructions need to be able to provide their source/destination registers.

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
  // Structural hazard: use port 2 so we defer priority to execute stage.
  core().readCMT(index, REGISTER_PORT_2);
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
