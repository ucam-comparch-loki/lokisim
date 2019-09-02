/*
 * PipelineStage.cpp
 *
 *  Created on: Aug 14, 2019
 *      Author: db434
 */

#include "PipelineStage.h"
#include "Core.h"
#include "../../Utility/Assert.h"

namespace Compute {

PipelineStage::PipelineStage(sc_module_name name) :
    LokiComponent(name),
    CoreInterface() {

}

void PipelineStage::readRegister(RegisterIndex index, RegisterPort port) {
  LOKI_ERROR << "Can't read registers from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::writeRegister(RegisterIndex index, int32_t value) {
  LOKI_ERROR << "Can't write registers from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::readPredicate() {
  LOKI_ERROR << "Can't read predicate from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::writePredicate(bool value) {
  LOKI_ERROR << "Can't write predicate from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::fetch(MemoryAddr address,
                           ChannelMapEntry::MemoryChannel channel,
                           bool execute, bool persistent) {
  LOKI_ERROR << "Can't fetch from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::jump(JumpOffset offset) {
  LOKI_ERROR << "Can't jump from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::computeLatency(opcode_t opcode, function_t fn) {
  LOKI_ERROR << "Can't compute in " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::readCMT(RegisterIndex index) {
  LOKI_ERROR << "Can't read channel map table from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::writeCMT(RegisterIndex index, EncodedCMTEntry value) {
  LOKI_ERROR << "Can't write channel map table from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::readCreg(RegisterIndex index) {
  LOKI_ERROR << "Can't read control registers from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::writeCreg(RegisterIndex index, int32_t value) {
  LOKI_ERROR << "Can't write control registers from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::readScratchpad(RegisterIndex index) {
  LOKI_ERROR << "Can't read scratchpad from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::writeScratchpad(RegisterIndex index, int32_t value) {
  LOKI_ERROR << "Can't write scratchpad from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::syscall(int code) {
  LOKI_ERROR << "Can't make system calls from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::waitForCredit(ChannelIndex channel) {
  LOKI_ERROR << "Can't wait for credits from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::sendOnNetwork(NetworkData flit) {
  LOKI_ERROR << "Can't send on network from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::selectChannelWithData(uint bitmask) {
  LOKI_ERROR << "Can't select input channel from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::startRemoteExecution(ChannelID address) {
  LOKI_ERROR << "Can't start remote execution from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::endRemoteExecution() {
  LOKI_ERROR << "Can't end remote execution from " << this->name() << endl;
  loki_assert(false);
}

ChannelID PipelineStage::getNetworkDestination(EncodedCMTEntry channelMap,
                                               MemoryAddr address) const {
  return core().getNetworkDestination(channelMap, address);
}

bool PipelineStage::inputFIFOHasData(ChannelIndex fifo) const {
  return core().inputFIFOHasData(fifo);
}

uint PipelineStage::creditsAvailable(ChannelIndex channel) const {
  return core().creditsAvailable(channel);
}

} // end namespace
