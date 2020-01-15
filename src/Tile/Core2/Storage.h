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

#include "StorageBase.h"
#include "../../ISA/CoreInterface.h"
#include "../../Network/FIFOs/NetworkFIFO.h"
#include "../../Network/Interface.h"
#include "../ChannelMapEntry.h"

namespace Compute {

class Core;
using sc_core::sc_event;

// Implementation of StorageBase which holds its data in an array.
template<typename stored_type,
         typename read_type=stored_type,
         typename write_type=stored_type>
class RegisterFileBase : public StorageBase<stored_type, read_type, write_type> {

//============================================================================//
// Constructors and destructors
//============================================================================//

protected:
  RegisterFileBase(sc_module_name name, uint readPorts, uint writePorts,
                   size_t size) :
      StorageBase<stored_type, read_type, write_type>(name, readPorts, writePorts),
      data(size) {
    // Nothing
  }

//============================================================================//
// Methods
//============================================================================//

public:

  // Read which bypasses all normal processes and completes immediately.
  const stored_type& debugRead(RegisterIndex reg) {
    return data[reg];
  }
  const stored_type& debugRead(RegisterIndex reg) const {
    return data[reg];
  }

  // Write which bypasses all normal processes and completes immediately.
  void debugWrite(RegisterIndex reg, stored_type value) {
    data[reg] = value;
  }

protected:

  void processRequests() {
    if (!this->clock.posedge()) {
      next_trigger(this->clock.posedge_event());
      return;
    }

    // Process writes first so reads see the latest data.
    for (uint port=0; port<this->writePort.size(); port++) {
      if (this->writePort[port].inProgress()) {
        doWrite(port);
        this->writePort[port].notifyFinished();
      }
    }

    for (uint port=0; port<this->readPort.size(); port++)
      if (this->readPort[port].inProgress())
        doRead(port);
  }

  // TODO: virtual? This should almost certainly be overridden if stored_type
  // != write_type.
  void doWrite(uint port) {
    // Default: copy data from the port to the data store.
    data[this->writePort[port].reg()] = this->writePort[port].result();
  }

  // TODO: virtual? This should almost certainly be overridden if stored_type
  // != read_type.
  void doRead(uint port) {
    // Default: copy data from the data store to the port.
    this->readPort[port].setResult(data[this->readPort[port].reg()]);
  }

  const Core& core() const;
  const ComponentID& id() const;

//============================================================================//
// Local state
//============================================================================//

protected:

  vector<stored_type> data;

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

class ChannelMapTable : public RegisterFileBase<ChannelMapEntry, EncodedCMTEntry, EncodedCMTEntry> {
public:
  // Connection from the global credit network.
  typedef sc_port<network_sink_ifc<Word>> InPort;
  InPort iCredit;

  SC_HAS_PROCESS(ChannelMapTable);
  ChannelMapTable(sc_module_name name,
                  const channel_map_table_parameters_t& params);
  uint creditsAvailable(ChannelIndex channel) const;
  const sc_event& creditArrivedEvent() const;
protected:
  void doRead(uint port);
  void doWrite(uint port);
private:
  void consumeCredit();

  NetworkFIFO<Word> iCreditFIFO;
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
