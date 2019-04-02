/*
 * Interface.h
 *
 *  Created on: 24 Mar 2019
 *      Author: db434
 */

#ifndef SRC_NETWORK_INTERFACE_H_
#define SRC_NETWORK_INTERFACE_H_

#include <systemc>
#include "../Datatype/Flit.h"

using sc_core::sc_event;
using sc_core::sc_interface;

template<typename T>
class network_source_ifc : virtual public sc_interface {
public:
  // Read a flit into the network. Consume the source's copy.
  virtual const Flit<T> read() = 0;

  // Look at the next flit to be sent. Do not consume it.
  virtual const Flit<T> peek() const = 0;

  // Tell whether data is ready to be sent.
  virtual bool dataAvailable() const = 0;

  // Event which is triggered whenever new data arrives.
  virtual const sc_event& dataAvailableEvent() const = 0;

  // For debug, return the most-recently read data. If any other writes
  // have happened since the read, the result is not guaranteed to be correct.
  virtual const Flit<T> lastDataRead() const = 0;
};

template<typename T>
class network_sink_ifc : virtual public sc_interface {
public:
  // Write data to the network's output.
  virtual void write(const Flit<T>& data) = 0;

  // TODO: allow this single output to cover multiple buffers.
  //   e.g. Add an optional parameter specifying which address/buffer we want.
  // Tell whether the output is able to receive data.
  virtual bool canWrite() const = 0;

  // Event which is triggered whenever data is consumed by the sink.
  virtual const sc_event& canWriteEvent() const = 0;

  // For debug, return the most-recently written data.
  virtual const Flit<T> lastDataWritten() const = 0;
};

// Combination of the two.
template<typename T>
class network_inout_ifc : virtual public network_source_ifc<T>,
                          virtual public network_sink_ifc<T> {
  // Nothing extra.
};

#endif /* SRC_NETWORK_INTERFACE_H_ */
