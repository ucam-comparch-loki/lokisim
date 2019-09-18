/*
 * NetworkChannel.h
 *
 * Start/end point for a Network. Has zero latency and a capacity of one value.
 * Behaves a lot like a sc_signal, but does not allow data to be overwritten
 * until it has been consumed.
 *
 *  Created on: Sep 17, 2019
 *      Author: db434
 */

#ifndef SRC_NETWORK_NETWORKCHANNEL_H_
#define SRC_NETWORK_NETWORKCHANNEL_H_

#include "../LokiComponent.h"
#include "../Utility/Assert.h"
#include "Interface.h"

template<class T>
class NetworkChannel : public LokiComponent,
                       public network_inout_ifc<T> {

public:
  typedef Flit<T> stored_data;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  NetworkChannel(sc_module_name name) :
      LokiComponent(name) {
    valid = false;
  }

  virtual ~NetworkChannel() {}

//============================================================================//
// Methods
//============================================================================//

public:
  // Read a flit into the network. Consume the source's copy.
  virtual const Flit<T> read() {
    loki_assert(canRead());

    readEvt.notify(sc_core::SC_ZERO_TIME);
    valid = false;
    previousData = data;
    return data;
  }

  // Look at the next flit to be sent. Do not consume it.
  virtual const Flit<T> peek() const {
    loki_assert(canRead());
//    previousData = data;  // Can't do this in const method
    return data;
  }

  // Tell whether data is ready to be sent.
  virtual bool canRead() const {
    return valid;
  }

  // Event which is triggered whenever new data is available to read. This is
  // subtly different from writeEvent() in that it may also be triggered if a
  // read reveals new data.
  virtual const sc_event& canReadEvent() const {
    return writeEvt;
  }

  // Event which is triggered whenever new data arrives. For debug only.
  virtual const sc_event& writeEvent() const {
    return writeEvt;
  }

  // For debug, return the most-recently read data. If any other writes
  // have happened since the read, the result is not guaranteed to be correct.
  virtual const Flit<T> lastDataRead() const {
    return previousData;
  }

  // Write data to the network's output.
  virtual void write(const Flit<T>& data) {
    loki_assert(canWrite());

    writeEvt.notify(sc_core::SC_ZERO_TIME);
    this->data = data;
    valid = true;
  }

  // Tell whether the output is able to receive data.
  virtual bool canWrite() const {
    return !valid;
  }

  // Event which is triggered whenever data is read by the sink.
  virtual const sc_event& canWriteEvent() const {
    return readEvt;
  }

  // Event which is triggered whenever data is consumed by the sink. This is
  // subtly different from canWriteEvent() because some components may read a
  // single value multiple times. Values may only be consumed once, however.
  virtual const sc_event& dataConsumedEvent() const {
    return readEvt;
  }

  // For debug, return the most-recently written data.
  virtual const Flit<T> lastDataWritten() const {
    return data;
  }

//============================================================================//
// Local state
//============================================================================//

private:

  stored_data data;
  bool valid;

  stored_data previousData; // For debug only

  sc_event readEvt, writeEvt;

};



#endif /* SRC_NETWORK_NETWORKCHANNEL_H_ */
