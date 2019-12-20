/*
 * Storage.h
 *
 * All significant storage elements in the core:
 *  * Register file
 *  * Scratchpad
 *  * Channel map table
 *  * Control registers
 *  * Predicate register
 *
 *  Created on: Aug 16, 2019
 *      Author: db434
 */

#ifndef SRC_TILE_CORE_STORAGE_H_
#define SRC_TILE_CORE_STORAGE_H_

#include "../../ISA/CoreInterface.h"
#include "../ChannelMapEntry.h"
#include "StorageBase.h"

namespace Compute {

// Implementation of StorageBase which holds its data in an array.
template<typename T>
class RegisterFileBase : public StorageBase<T> {

//============================================================================//
// Constructors and destructors
//============================================================================//

protected:
  RegisterFileBase(sc_module_name name, uint readPorts, uint writePorts,
                   size_t size) :
      StorageBase<T>(name, readPorts, writePorts),
      data(size) {
    // Nothing
  }

//============================================================================//
// Methods
//============================================================================//

public:

  // Read which bypasses all normal processes and completes immediately.
  const T& debugRead(RegisterIndex reg) {
    return data[reg];
  }
  const T& debugRead(RegisterIndex reg) const {
    return data[reg];
  }

  // Write which bypasses all normal processes and completes immediately.
  void debugWrite(RegisterIndex reg, T value) {
    data[reg] = value;
  }

protected:

  void processRequests() {
    if (!this->clock.posedge()) {
      next_trigger(this->clock.posedge_event());
      return;
    }

    // Process writes first so reads see the latest data.
    for (uint i=0; i<this->writePort.size(); i++) {
      if (this->writePort[i].inProgress()) {
        data[this->writePort[i].reg()] = this->writePort[i].result();
        this->writePort[i].notifyFinished();
      }
    }

    for (uint i=0; i<this->readPort.size(); i++)
      if (this->readPort[i].inProgress())
        this->readPort[i].setResult(data[this->readPort[i].reg()]);
  }

//============================================================================//
// Local state
//============================================================================//

protected:

  vector<T> data;

};



class RegisterFile : public RegisterFileBase<int32_t> {
public:
  RegisterFile(sc_module_name name, const register_file_parameters_t& params);
  void read(RegisterIndex reg, RegisterPort port);
};

class Scratchpad : public RegisterFileBase<int32_t> {
public:
  Scratchpad(sc_module_name name, const scratchpad_parameters_t& params);
};

class ChannelMapTable : public RegisterFileBase<ChannelMapEntry> {
public:
  ChannelMapTable(sc_module_name name,
                  const channel_map_table_parameters_t& params);
  void write(RegisterIndex index, EncodedCMTEntry value);
  uint creditsAvailable(ChannelIndex channel) const;
};

class ControlRegisters : public RegisterFileBase<int32_t> {
public:
  ControlRegisters(sc_module_name name);
};

class PredicateRegister: public RegisterFileBase<bool> {
public:
  PredicateRegister(sc_module_name name);
  void read();
  void write(bool value);
};

} // end namespace

#endif /* SRC_TILE_CORE_STORAGE_H_ */
