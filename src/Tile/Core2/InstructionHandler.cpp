/*
 * InstructionHandler.cpp
 *
 *  Created on: Aug 14, 2019
 *      Author: db434
 */

#include "InstructionHandler.h"
#include "../../Utility/Assert.h"
#include "Core.h"
#include "PipelineStage.h"

namespace Compute {

InstructionHandler::InstructionHandler(sc_module_name name) :
    LokiComponent(name) {
  active = false;

  SC_METHOD(mainLoop);
  sensitive << beginEvent;
  dont_initialize();
}

void InstructionHandler::begin(DecodedInstruction inst) {
  currentInstruction = inst;
  beginEvent.notify(sc_core::SC_ZERO_TIME);
}

void InstructionHandler::respondToInstruction(DecodedInstruction inst) const {
  loki_assert(false);
}

const sc_event& InstructionHandler::coreFinishedEvent() const {
  loki_assert(false);
  return *(new sc_event());
}

PipelineStage& InstructionHandler::stage() const {
  return static_cast<PipelineStage&>(*(this->get_parent_object()));
}

Core& InstructionHandler::core() const {
  return stage().core();
}

void InstructionHandler::mainLoop() {
  if (active)
    respondToInstruction(currentInstruction);
    // Default trigger: instruction makes new request.
  else
    next_trigger(coreFinishedEvent());

  active = !active;
}


ReadRegisterHandler::ReadRegisterHandler(sc_module_name name, RegisterPort port) :
    InstructionHandler(name),
    port(port) {
  // Nothing
}

void ReadRegisterHandler::respondToInstruction(DecodedInstruction inst) const {
  int32_t result = core().getRegisterOutput(port);
  inst->readRegistersCallback(port, result);
}

const sc_event& ReadRegisterHandler::coreFinishedEvent() const {
  return core().readRegistersEvent(port);
}


WriteRegisterHandler::WriteRegisterHandler(sc_module_name name) :
    InstructionHandler(name) {
  // Nothing
}

void WriteRegisterHandler::respondToInstruction(DecodedInstruction inst) const {
  inst->writeRegistersCallback();
}

const sc_event& WriteRegisterHandler::coreFinishedEvent() const {
  return core().wroteRegistersEvent();
}


ComputeHandler::ComputeHandler(sc_module_name name) :
    InstructionHandler(name) {
  // Nothing
}

void ComputeHandler::respondToInstruction(DecodedInstruction inst) const {
  inst->computeCallback();
}

const sc_event& ComputeHandler::coreFinishedEvent() const {
  // TODO: there are multiple possibilities here. e.g. woche and selch wait for
  // network things to happen.
  return core().computeFinishedEvent();
}


ReadCMTHandler::ReadCMTHandler(sc_module_name name, RegisterPort port) :
    InstructionHandler(name),
    port(port) {
  // Nothing
}

void ReadCMTHandler::respondToInstruction(DecodedInstruction inst) const {
  EncodedCMTEntry result = core().getCMTOutput(port);
  inst->readCMTCallback(result);
}

const sc_event& ReadCMTHandler::coreFinishedEvent() const {
  return core().readCMTEvent(port);
}


WriteCMTHandler::WriteCMTHandler(sc_module_name name) :
    InstructionHandler(name) {
  // Nothing
}

void WriteCMTHandler::respondToInstruction(DecodedInstruction inst) const {
  inst->writeCMTCallback();
}

const sc_event& WriteCMTHandler::coreFinishedEvent() const {
  return core().wroteCMTEvent();
}


ReadScratchpadHandler::ReadScratchpadHandler(sc_module_name name) :
    InstructionHandler(name) {
  // Nothing
}

void ReadScratchpadHandler::respondToInstruction(DecodedInstruction inst) const {
  int32_t result = core().getScratchpadOutput();
  inst->readScratchpadCallback(result);
}

const sc_event& ReadScratchpadHandler::coreFinishedEvent() const {
  return core().readScratchpadEvent();
}


WriteScratchpadHandler::WriteScratchpadHandler(sc_module_name name) :
    InstructionHandler(name) {
  // Nothing
}

void WriteScratchpadHandler::respondToInstruction(DecodedInstruction inst) const {
  inst->writeScratchpadCallback();
}

const sc_event& WriteScratchpadHandler::coreFinishedEvent() const {
  return core().wroteScratchpadEvent();
}


ReadCRegsHandler::ReadCRegsHandler(sc_module_name name) :
    InstructionHandler(name) {
  // Nothing
}

void ReadCRegsHandler::respondToInstruction(DecodedInstruction inst) const {
  int32_t result = core().getCRegOutput();
  inst->readCregsCallback(result);
}

const sc_event& ReadCRegsHandler::coreFinishedEvent() const {
  return core().readCRegsEvent();
}


WriteCRegsHandler::WriteCRegsHandler(sc_module_name name) :
    InstructionHandler(name) {
  // Nothing
}

void WriteCRegsHandler::respondToInstruction(DecodedInstruction inst) const {
  inst->writeCregsCallback();
}

const sc_event& WriteCRegsHandler::coreFinishedEvent() const {
  return core().wroteCRegsEvent();
}


ReadPredicateHandler::ReadPredicateHandler(sc_module_name name) :
    InstructionHandler(name) {
  // Nothing
}

void ReadPredicateHandler::respondToInstruction(DecodedInstruction inst) const {
  bool result = core().getPredicateOutput();
  inst->readCregsCallback(result);
}

const sc_event& ReadPredicateHandler::coreFinishedEvent() const {
  return core().readPredicateEvent();
}


WritePredicateHandler::WritePredicateHandler(sc_module_name name) :
    InstructionHandler(name) {
  // Nothing
}

void WritePredicateHandler::respondToInstruction(DecodedInstruction inst) const {
  inst->writePredicateCallback();
}

const sc_event& WritePredicateHandler::coreFinishedEvent() const {
  return core().wrotePredicateEvent();
}


NetworkSendHandler::NetworkSendHandler(sc_module_name name) :
    InstructionHandler(name) {
  // Nothing
}

void NetworkSendHandler::respondToInstruction(DecodedInstruction inst) const {
  inst->sendNetworkDataCallback();
}

const sc_event& NetworkSendHandler::coreFinishedEvent() const {
  return core().sentNetworkDataEvent();
}


WocheHandler::WocheHandler(sc_module_name name) :
    InstructionHandler(name) {
  // Nothing
}

void WocheHandler::respondToInstruction(DecodedInstruction inst) const {
  inst->creditArrivedCallback();
}

const sc_event& WocheHandler::coreFinishedEvent() const {
  // A credit arriving may not mean that woche is finished - this is managed
  // in the woche instruction.
  return core().creditArrivedEvent();
}


SelchHandler::SelchHandler(sc_module_name name) :
    InstructionHandler(name) {
  // Nothing
}

void SelchHandler::respondToInstruction(DecodedInstruction inst) const {
  int32_t bufferIndex = core().getSelectedChannel();
  inst->computeCallback(bufferIndex);
}

const sc_event& SelchHandler::coreFinishedEvent() const {
  return core().selchFinishedEvent();
}

} // end namespace
