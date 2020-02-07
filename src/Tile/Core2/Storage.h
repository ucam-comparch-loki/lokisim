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

  typedef StorageBase<stored_type, read_type, write_type> parent_t;

//============================================================================//
// Constructors and destructors
//============================================================================//

protected:
  RegisterFileBase(sc_module_name name, uint readPorts, uint writePorts,
                   size_t size, bool prioritiseReads=false,
                   bool prioritiseWrites=false) :
      parent_t(name, readPorts, writePorts),
      data(size),
      prioritiseReads(prioritiseReads),
      prioritiseWrites(prioritiseWrites) {
    // Nothing
  }

//============================================================================//
// Methods
//============================================================================//

public:

  // Read which bypasses all normal processes and completes immediately.
  const read_type debugRead(RegisterIndex reg);
  const read_type debugRead(RegisterIndex reg) const;

  // Write which bypasses all normal processes and completes immediately.
  void debugWrite(RegisterIndex reg, write_type value);

protected:

  void processRequests();

  // Let an instruction know that its requested write operation has completed.
  virtual void notifyWriteFinished(DecodedInstruction inst, PortIndex port) = 0;

  // Let an instruction know that its requested read operation has completed.
  virtual void notifyReadFinished(DecodedInstruction inst, PortIndex port,
                                  read_type result) = 0;

  virtual void doWrite(PortIndex port);
  virtual void doRead(PortIndex port);

  const Core& core() const;
  const ComponentID& id() const;

//============================================================================//
// Local state
//============================================================================//

protected:

  vector<stored_type> data;

private:

  // If true, allow only one read/write per clock cycle. Priority is given to
  // the port with the lowest index.
  const bool prioritiseReads, prioritiseWrites;

};



class RegisterFile : public RegisterFileBase<int32_t> {
public:
  RegisterFile(sc_module_name name,
               const register_file_parameters_t& params,
               uint numFIFOs);
  void read(DecodedInstruction inst, RegisterIndex reg, PortIndex port);

  bool isReadOnly(RegisterIndex reg) const;
  bool isFIFOMapped(RegisterIndex reg) const;
  bool isStandard(RegisterIndex reg) const; // i.e. not read-only or a FIFO
protected:
  virtual void notifyWriteFinished(DecodedInstruction inst, PortIndex port);
  virtual void notifyReadFinished(DecodedInstruction inst, PortIndex port,
                                  read_t result);
private:
  const uint numFIFOs;
};


class Scratchpad : public RegisterFileBase<int32_t> {
public:
  Scratchpad(sc_module_name name, const scratchpad_parameters_t& params);
protected:
  virtual void notifyWriteFinished(DecodedInstruction inst, PortIndex port);
  virtual void notifyReadFinished(DecodedInstruction inst, PortIndex port,
                                  read_t result);
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
  void waitForCredit(DecodedInstruction inst, ChannelIndex channel);
protected:
  virtual void doRead(PortIndex port);
  virtual void doWrite(PortIndex port);
  virtual void notifyWriteFinished(DecodedInstruction inst, PortIndex port);
  virtual void notifyReadFinished(DecodedInstruction inst, PortIndex port,
                                  read_t result);
private:
  void consumeCredit();
  void notifyWaitingInstruction();

  NetworkFIFO<Word> iCreditFIFO;

  // At most one instruction can be waiting for credits at a time.
  DecodedInstruction instruction;
  ChannelIndex blockingChannel;
};


class ControlRegisters : public RegisterFileBase<int32_t> {
public:
  ControlRegisters(sc_module_name name);
protected:
  virtual void notifyWriteFinished(DecodedInstruction inst, PortIndex port);
  virtual void notifyReadFinished(DecodedInstruction inst, PortIndex port,
                                  read_t result);
};


class PredicateRegister: public RegisterFileBase<bool> {
public:
  PredicateRegister(sc_module_name name);
  void read(DecodedInstruction inst);
  void write(DecodedInstruction inst, bool value);
protected:
  virtual void notifyWriteFinished(DecodedInstruction inst, PortIndex port);
  virtual void notifyReadFinished(DecodedInstruction inst, PortIndex port,
                                  read_t result);
};

} // end namespace

#endif /* SRC_TILE_CORE_STORAGE_H_ */
