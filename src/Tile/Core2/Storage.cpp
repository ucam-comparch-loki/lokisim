/*
 * Storage.cpp
 *
 *  Created on: Aug 16, 2019
 *      Author: db434
 */

#include "Storage.h"

#include "../../Utility/Assert.h"
#include "../../Utility/Parameters.h"

namespace Compute {

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
    RegisterFileBase<ChannelMapEntry>(name, 1, 1, params.size) {
  // Nothing
}

void ChannelMapTable::write(RegisterIndex index, EncodedCMTEntry value) {
  // TODO: ChannelMapEntry contains lots of state. Can't update all of it here?
  RegisterFileBase<ChannelMapEntry>::write(index, value);
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
