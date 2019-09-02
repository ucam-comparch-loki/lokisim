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

#ifndef SRC_TILE_CORE2_STORAGEBASE_H_
#define SRC_TILE_CORE2_STORAGEBASE_H_

#include "../../ISA/CoreInterface.h"
#include "../../LokiComponent.h"
#include "../../Utility/Assert.h"
#include "../../Utility/LokiVector.h"

// This looks a lot like an sc_signal. Is there an advantage to having this
// class?
template<typename T>
class StorageReadPort : public LokiComponent {
public:
  StorageReadPort(sc_module_name name) :
      LokiComponent(name) {
    toRead = -1;
    requestInProgress = false;
  }

  void newRequest(RegisterIndex reg) {
    loki_assert(!requestInProgress);
    toRead = reg;
    requestInProgress = true;
    requestArrivedEvent.notify(sc_core::SC_ZERO_TIME);
  }

  void setResult(T value) {
    loki_assert(requestInProgress);
    data = value;
    requestInProgress = false;
    finishedEvent.notify(sc_core::SC_ZERO_TIME);
  }

  RegisterIndex reg() const {return toRead;}
  bool inProgress() const {return requestInProgress;}
  T result() const {return data;}
  const sc_core::sc_event& finished() const {return finishedEvent;}
  const sc_core::sc_event& requestArrived() const {return requestArrivedEvent;}

private:
  RegisterIndex toRead;
  bool requestInProgress;
  T data;

  sc_core::sc_event requestArrivedEvent;
  sc_core::sc_event finishedEvent;
};

template<typename T>
class StorageWritePort : public LokiComponent {
public:
  StorageWritePort(sc_module_name name) :
      LokiComponent(name) {
    toWrite = -1;
    requestInProgress = false;
  }

  void newRequest(RegisterIndex reg, T value) {
    loki_assert(!requestInProgress);
    toWrite = reg;
    data = value;
    requestInProgress = true;
    requestArrivedEvent.notify(sc_core::SC_ZERO_TIME);
  }

  void notifyFinished() {
    loki_assert(requestInProgress);
    requestInProgress = false;
    finishedEvent.notify(sc_core::SC_ZERO_TIME);
  }

  RegisterIndex reg() const {return toWrite;}
  bool inProgress() const {return requestInProgress;}
  T result() const {return data;}
  const sc_core::sc_event& finished() const {return finishedEvent;}
  const sc_core::sc_event& requestArrived() const {return requestArrivedEvent;}

private:
  RegisterIndex toWrite;
  bool requestInProgress;
  T data;

  sc_core::sc_event requestArrivedEvent;
  sc_core::sc_event finishedEvent;
};


template<typename stored_type,
         typename read_type=stored_type,
         typename write_type=stored_type>
class StorageBase : public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput clock;

  // Not sure if I want full SystemC ports for inputs and outputs, or something
  // simpler.
  LokiVector<StorageReadPort<read_type>> readPort;
  LokiVector<StorageWritePort<write_type>> writePort;

//============================================================================//
// Constructors and destructors
//============================================================================//

protected:

  SC_HAS_PROCESS(StorageBase);
  StorageBase(sc_module_name name, uint readPorts, uint writePorts) :
      LokiComponent(name),
      clock("clock"),
      readPort("read_port", readPorts),
      writePort("write_port", writePorts) {

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
  void read(RegisterIndex reg, RegisterPort port=REGISTER_PORT_1) {
    readPort[port].newRequest(reg);
  }

  // Write a register. The process is asynchronous. When complete, an event
  // will be triggered in writePort[port].
  void write(RegisterIndex reg, write_type value, RegisterPort port=REGISTER_PORT_1) {
    writePort[port].newRequest(reg, value);
  }

  // Read which bypasses all normal processes and completes immediately.
  const read_type& debugRead(RegisterIndex reg) {
    loki_assert(false);
    return *(new read_type());
  }

  // Write which bypasses all normal processes and completes immediately.
  void debugWrite(RegisterIndex reg, write_type value) {
    loki_assert(false);
  }

protected:

  void processRequests() {
    loki_assert(false);
  }

};


#endif /* SRC_TILE_CORE_STORAGEBASE_H_ */
