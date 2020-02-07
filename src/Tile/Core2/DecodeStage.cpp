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
    MiddlePipelineStage(name) {

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

void DecodeStage::readRegister(RegisterIndex index, PortIndex port) {
  core().readRegister(instruction, index, port);
}

void DecodeStage::readPredicate() {
  core().readPredicate(instruction);
}

void DecodeStage::readCMT(RegisterIndex index) {
  // Structural hazard: use port 1 so we defer priority to execute stage.
  core().readCMT(instruction, index, 1);
}

void DecodeStage::fetch(MemoryAddr address,
                        ChannelMapEntry::MemoryChannel channel,
                        bool execute, bool persistent) {
  core().fetch(instruction, address, channel, execute, persistent);
}

void DecodeStage::jump(JumpOffset offset) {
  core().jump(instruction, offset);
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
  core().waitForCredit(instruction, channel);
}

void DecodeStage::selectChannelWithData(uint bitmask) {
  core().selectChannelWithData(instruction, bitmask);
}

} // end namespace
