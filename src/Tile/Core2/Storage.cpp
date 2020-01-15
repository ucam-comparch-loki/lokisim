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
                           const register_file_parameters_t& params) :
    RegisterFileBase<int32_t>(name, 2, 1, params.size) {
  // Nothing
}

void RegisterFile::read(RegisterIndex reg, RegisterPort port) {
  RegisterFileBase<int32_t>::read(reg, port);

  // Some of the registers are hard-wired, so access those immediately.
  switch (reg) {
    // Constant zero.
    case 0: readPort[port].setResult(0); break;

    // Current instruction packet address.
    case 1: readPort[port].setResult(data[1]); break;

    // Network FIFOs.
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      loki_assert(false); break;

    // All other registers: wait for processRequests to kick in.
    default: break;
  }
}


Scratchpad::Scratchpad(sc_module_name name,
                       const scratchpad_parameters_t& params) :
    RegisterFileBase<int32_t>(name, 1, 1, params.size) {
  // Nothing
}


ChannelMapTable::ChannelMapTable(sc_module_name name,
                                 const channel_map_table_parameters_t& params) :
    RegisterFileBase<ChannelMapEntry, EncodedCMTEntry, EncodedCMTEntry>(name, 1, 1, 0),
    iCreditFIFO("credits", 1) {  // More of a register than a FIFO

  // Need to bypass the normal constructor so we can provide arguments when
  // initialising stored data. Argument = network address on credit network.
  ComponentID thisCore = id();
  for (uint i=0; i<params.size; i++)
    data.push_back(ChannelMapEntry(ChannelID(thisCore, i)));

  iCredit(iCreditFIFO);

  SC_METHOD(consumeCredit);
  sensitive << iCreditFIFO.canReadEvent();
  dont_initialize();

}

uint ChannelMapTable::creditsAvailable(ChannelIndex channel) const {
  return data[channel].numCredits();
}

const sc_core::sc_event& ChannelMapTable::creditArrivedEvent() const {
  return iCreditFIFO.dataConsumedEvent();
}

void ChannelMapTable::doRead(uint port) {
  // Convert from ChannelMapEntry to EncodedCMTEntry.
  EncodedCMTEntry result = data[this->readPort[port].reg()].read();
  this->readPort[port].setResult(result);
}

void ChannelMapTable::doWrite(uint port) {
  // Convert from EncodedCMTEntry to ChannelMapEntry.
  EncodedCMTEntry encoded = this->writePort[port].result();
  data[this->writePort[port].reg()].write(encoded);
}

void ChannelMapTable::consumeCredit() {
  NetworkCredit credit = iCreditFIFO.read();

  ChannelIndex channel = credit.channelID().channel;
  uint numCredits = credit.payload().toUInt();

  LOKI_LOG(3) << this->name() << " received " << numCredits << " credit(s) at "
      << ChannelID(id(), channel) << " " << credit.messageID() << endl;

  data[channel].addCredits(numCredits);
}


ControlRegisters::ControlRegisters(sc_module_name name) :
    RegisterFileBase<int32_t>(name, 1, 1, 16) {
  // Nothing
}


PredicateRegister::PredicateRegister(sc_module_name name) :
    RegisterFileBase<bool>(name, 1, 1, 1) {
  // Nothing
}

void PredicateRegister::read() {
  RegisterFileBase<bool>::read(0);
}

void PredicateRegister::write(bool value) {
  RegisterFileBase<bool>::write(0, value);
}

} // end namespace
