/*
 * Storage.cpp
 *
 *  Created on: Aug 16, 2019
 *      Author: db434
 */

#include "Storage.h"
#include "Core.h"
#include "../../Utility/Assert.h"
#include "../../Utility/Parameters.h"

namespace Compute {

template<typename stored_type, typename read_type, typename write_type>
const read_type RegisterFileBase<stored_type, read_type, write_type>::debugRead(RegisterIndex reg) {
  return doRead(reg);
}

template<typename stored_type, typename read_type, typename write_type>
const read_type RegisterFileBase<stored_type, read_type, write_type>::debugRead(RegisterIndex reg) const {
  return doRead(reg);
}

template<typename stored_type, typename read_type, typename write_type>
void RegisterFileBase<stored_type, read_type, write_type>::debugWrite(RegisterIndex reg, write_type value) {
  doWrite(reg, value);
}

template<typename stored_type, typename read_type, typename write_type>
void RegisterFileBase<stored_type, read_type, write_type>::processRequests() {
  if (!this->clock.posedge()) {
    next_trigger(this->clock.posedge_event());
    return;
  }

  // Process writes first so reads see the latest data.
  for (uint port=0; port<this->writePort.size(); port++) {
    if (this->writePort[port].inProgress()) {
      RegisterIndex reg = this->writePort[port].reg();
      write_type value = this->writePort[port].result();
      doWrite(reg, value);
      notifyWriteFinished(this->writePort[port].instruction(), port);
      this->writePort[port].clear();

      if (prioritiseWrites) {
        next_trigger(this->clock.posedge_event());
        break;
      }
    }
  }

  for (uint port=0; port<this->readPort.size(); port++) {
    if (this->readPort[port].inProgress()) {
      RegisterIndex reg = this->readPort[port].reg();
      read_type result = doRead(reg);
      notifyReadFinished(this->readPort[port].instruction(), port, result);
      this->readPort[port].clear();

      if (prioritiseReads) {
        next_trigger(this->clock.posedge_event());
        break;
      }
    }
  }
}

template<typename stored_type, typename read_type, typename write_type>
void RegisterFileBase<stored_type, read_type, write_type>::doWrite(RegisterIndex reg, write_type value) {
  // Default: copy data directly to storage.
  data[reg] = value;
}

template<typename stored_type, typename read_type, typename write_type>
read_type RegisterFileBase<stored_type, read_type, write_type>::doRead(RegisterIndex reg) {
  // Default: copy data directly from storage.
  return data[reg];
}

template<typename stored_type, typename read_type, typename write_type>
read_type RegisterFileBase<stored_type, read_type, write_type>::doRead(RegisterIndex reg) const {
  // Default: copy data directly from storage.
  return data[reg];
}

// Assuming all instances are direct submodules of Core.
template<typename stored_type, typename read_type, typename write_type>
const Core& RegisterFileBase<stored_type, read_type, write_type>::core() const {
  return static_cast<Core&>(*(this->get_parent_object()));
}

template<typename stored_type, typename read_type, typename write_type>
const ComponentID& RegisterFileBase<stored_type, read_type, write_type>::id() const {
  return core().id;
}

RegisterFile::RegisterFile(sc_module_name name,
                           const register_file_parameters_t& params,
                           uint numFIFOs) :
    RegisterFileBase<int32_t>(name, 2, 1, params.size),
    numFIFOs(numFIFOs) {
  // Nothing
}

void RegisterFile::read(DecodedInstruction inst, RegisterIndex reg,
                        PortIndex port) {
  RegisterFileBase<int32_t>::read(inst, reg, port);

  // Some of the registers are hard-wired, so access those immediately.
  switch (reg) {
    // Constant zero.
    case 0: {
      read_t result = 0;
      notifyReadFinished(inst, port, result);
      this->readPort[port].clear();
      break;
    }

    // Current instruction packet address.
    case 1: {
      read_t result = data[1];
      notifyReadFinished(inst, port, result);
      this->readPort[port].clear();
      break;
    }

    // Network FIFOs.
    // TODO: use numFIFOs parameter.
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      loki_assert(false); break;

    // All other registers: everything is handled by the base class; wait for
    // processRequests to kick in.
    default: break;
  }
}

bool RegisterFile::isReadOnly(RegisterIndex reg) const {
  return reg < 2;
}

bool RegisterFile::isFIFOMapped(RegisterIndex reg) const {
  return (reg >= 2) && (reg < (2 + numFIFOs));
}

bool RegisterFile::isStandard(RegisterIndex reg) const {
  return reg >= (2 + numFIFOs);
}

void RegisterFile::notifyWriteFinished(DecodedInstruction inst, PortIndex port) {
  inst->writeRegistersCallback(port);
}

void RegisterFile::notifyReadFinished(DecodedInstruction inst, PortIndex port,
                                      read_t result) {
  inst->readRegistersCallback(port, result);
}


Scratchpad::Scratchpad(sc_module_name name,
                       const scratchpad_parameters_t& params) :
    RegisterFileBase<int32_t>(name, 1, 1, params.size) {
  // Nothing
}

void Scratchpad::notifyWriteFinished(DecodedInstruction inst, PortIndex port) {
  inst->writeScratchpadCallback(port);
}

void Scratchpad::notifyReadFinished(DecodedInstruction inst, PortIndex port,
                                    read_t result) {
  inst->readScratchpadCallback(port, result);
}


ChannelMapTable::ChannelMapTable(sc_module_name name,
                                 const channel_map_table_parameters_t& params) :
    RegisterFileBase<ChannelMapEntry, EncodedCMTEntry, EncodedCMTEntry>(name, 2, 1, 0, true, false),
    iCreditFIFO("credits", 1) {  // More of a register than a FIFO

  // There are two parts of the core which may need to read the CMT:
  //  * Reads which allow data to be sent on the network happen in Decode
  //  * Reads for the getchmap instruction happen in Execute
  // Two read ports are provided, but the Execute port is given priority.

  // Need to bypass the normal constructor so we can provide arguments when
  // initialising stored data. Argument = network address on credit network.
  ComponentID thisCore = id();
  for (uint i=0; i<params.size; i++)
    data.push_back(ChannelMapEntry(ChannelID(thisCore, i)));

  blockingChannel = -1;

  iCredit(iCreditFIFO);

  SC_METHOD(consumeCredit);
  sensitive << iCreditFIFO.canReadEvent();
  dont_initialize();

}

uint ChannelMapTable::creditsAvailable(ChannelIndex channel) const {
  return data[channel].numCredits();
}

void ChannelMapTable::waitForCredit(DecodedInstruction inst, ChannelIndex channel) {
  loki_assert(!instruction);

  instruction = inst;
  blockingChannel = channel;
}

ChannelMapTable::read_t ChannelMapTable::doRead(RegisterIndex reg) {
  // Convert from ChannelMapEntry to EncodedCMTEntry using read().
  return data[reg].read();
}

void ChannelMapTable::doWrite(RegisterIndex reg, write_t value) {
  // Convert from EncodedCMTEntry to ChannelMapEntry using write().
  data[reg].write(value);
}

void ChannelMapTable::notifyWriteFinished(DecodedInstruction inst, PortIndex port) {
  inst->writeCMTCallback(port);
}

void ChannelMapTable::notifyReadFinished(DecodedInstruction inst, PortIndex port,
                                         read_t result) {
  inst->readCMTCallback(port, result);
}

void ChannelMapTable::consumeCredit() {
  NetworkCredit credit = iCreditFIFO.read();

  ChannelIndex channel = credit.channelID().channel;
  uint numCredits = credit.payload().toUInt();

  LOKI_LOG(3) << this->name() << " received " << numCredits << " credit(s) at "
      << ChannelID(id(), channel) << " " << credit.messageID() << endl;

  data[channel].addCredits(numCredits);

  if (channel == blockingChannel)
    notifyWaitingInstruction();
}

void ChannelMapTable::notifyWaitingInstruction() {
  loki_assert(instruction);

  instruction->creditArrivedCallback();
  instruction.reset();
  blockingChannel = -1;
}


ControlRegisters::ControlRegisters(sc_module_name name) :
    RegisterFileBase<int32_t>(name, 1, 1, 16) {
  // Nothing
}

void ControlRegisters::notifyWriteFinished(DecodedInstruction inst, PortIndex port) {
  inst->writeCregsCallback(port);
}

void ControlRegisters::notifyReadFinished(DecodedInstruction inst, PortIndex port,
                                          read_t result) {
  inst->readCregsCallback(port, result);
}


PredicateRegister::PredicateRegister(sc_module_name name) :
    RegisterFileBase<bool>(name, 1, 1, 1) {
  // Nothing
}

void PredicateRegister::read(DecodedInstruction inst) {
  RegisterFileBase<bool>::read(inst, 0);
}

void PredicateRegister::write(DecodedInstruction inst, bool value) {
  RegisterFileBase<bool>::write(inst, 0, value);
}

void PredicateRegister::notifyWriteFinished(DecodedInstruction inst, PortIndex port) {
  inst->writePredicateCallback(port);
}

void PredicateRegister::notifyReadFinished(DecodedInstruction inst, PortIndex port,
                                           read_t result) {
  inst->readPredicateCallback(port, result);
}

} // end namespace
