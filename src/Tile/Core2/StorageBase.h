/*
 * StorageBase.h
 *
 * Base class for all storage components.
 *
 * Default behaviour is to receive requests, and process them all simultaneously
 * on the next positive clock edge.
 *
 *  Created on: Aug 16, 2019
 *      Author: db434
 */

#ifndef SRC_TILE_CORE_STORAGEBASE_H_
#define SRC_TILE_CORE_STORAGEBASE_H_

#include "../../ISA/CoreInterface.h"
#include "../../ISA/InstructionDecode.h"
#include "../../LokiComponent.h"
#include "../../Utility/Assert.h"
#include "../../Utility/LokiVector.h"

// This looks a lot like an sc_signal. Is there an advantage to having this
// class?
template<typename T>
class StorageReadPort : public LokiComponent {
public:
  StorageReadPort(sc_module_name name, PortIndex port) :
      LokiComponent(name),
      position(port) {
    toRead = -1;
  }

  void newRequest(DecodedInstruction inst, RegisterIndex reg) {
    loki_assert(!inProgress());

    client = inst;
    toRead = reg;
    requestArrivedEvent.notify(sc_core::SC_ZERO_TIME);
  }

  void clear() {
    loki_assert(inProgress());
    client.reset();
  }

  RegisterIndex reg() const {return toRead;}
  bool inProgress() const {return (bool)client;}
  T result() const {return data;}
  DecodedInstruction instruction() const {return client;}
  const sc_core::sc_event& finished() const {return finishedEvent;}
  const sc_core::sc_event& requestArrived() const {return requestArrivedEvent;}

private:
  const PortIndex position;

  DecodedInstruction client;
  RegisterIndex toRead;
  T data;

  sc_core::sc_event requestArrivedEvent;
  sc_core::sc_event finishedEvent;
};

template<typename T>
class StorageWritePort : public LokiComponent {
public:
  StorageWritePort(sc_module_name name, PortIndex port) :
      LokiComponent(name),
      position(port) {
    toWrite = -1;
  }

  void newRequest(DecodedInstruction inst, RegisterIndex reg, T value) {
    loki_assert(!inProgress());

    client = inst;
    toWrite = reg;
    data = value;
    requestArrivedEvent.notify(sc_core::SC_ZERO_TIME);
  }

  void clear() {
    loki_assert(inProgress());
    client.reset();
  }

  RegisterIndex reg() const {return toWrite;}
  bool inProgress() const {return (bool)client;}
  T result() const {return data;}
  DecodedInstruction instruction() const {return client;}
  const sc_core::sc_event& finished() const {return finishedEvent;}
  const sc_core::sc_event& requestArrived() const {return requestArrivedEvent;}

private:
  const PortIndex position;

  DecodedInstruction client;
  RegisterIndex toWrite;
  T data;

  sc_core::sc_event requestArrivedEvent;
  sc_core::sc_event finishedEvent;
};


template<typename stored_type,
         typename read_type=stored_type,
         typename write_type=stored_type>
class StorageBase : public LokiComponent {

public:

  typedef read_type read_t;
  typedef write_type write_t;

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput clock;

  LokiVector<StorageReadPort<read_t>> readPort;
  LokiVector<StorageWritePort<write_t>> writePort;

//============================================================================//
// Constructors and destructors
//============================================================================//

protected:

  SC_HAS_PROCESS(StorageBase);
  StorageBase(sc_module_name name, uint readPorts, uint writePorts) :
      LokiComponent(name),
      clock("clock") {

    // Need to make ports manually (they need an extra position argument).
    for (uint i=0; i<readPorts; i++) {
      StorageReadPort<read_t>* port =
          new StorageReadPort<read_t>(sc_gen_unique_name("read_port"), i);
      readPort.push_back(port);
    }
    for (uint i=0; i<writePorts; i++) {
      StorageWritePort<write_t>* port =
          new StorageWritePort<write_t>(sc_gen_unique_name("write_port"), i);
      writePort.push_back(port);
    }

    // Wake the method whenever a request arrives, but only process it on a
    // clock edge.
    SC_METHOD(processRequests);
    for (uint i=0; i<readPort.size(); i++)
      sensitive << readPort[i].requestArrived();
    for (uint i=0; i<writePort.size(); i++)
      sensitive << writePort[i].requestArrived();
    dont_initialize();

  }

//============================================================================//
// Methods
//============================================================================//

public:

  // Read a register. The process is asynchronous. The result will be returned
  // to readPort[port] at some point later, and an event triggered.
  void read(DecodedInstruction inst, RegisterIndex reg, PortIndex port=0) {
    readPort[port].newRequest(inst, reg);
  }

  // Write a register. The process is asynchronous. When complete, an event
  // will be triggered in writePort[port].
  void write(DecodedInstruction inst, RegisterIndex reg, write_t value,
             PortIndex port=0) {
    writePort[port].newRequest(inst, reg, value);
  }

  // Read which bypasses all normal processes and completes immediately.
  const read_t& debugRead(RegisterIndex reg) {
    loki_assert(false);
    return *(new read_t());
  }

  // Write which bypasses all normal processes and completes immediately.
  void debugWrite(RegisterIndex reg, write_t value) {
    loki_assert(false);
  }

protected:

  void processRequests() {
    loki_assert(false);
  }

};


#endif /* SRC_TILE_CORE_STORAGEBASE_H_ */
