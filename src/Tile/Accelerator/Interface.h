/*
 * Interface.h
 *
 * The main data interface between all compute and memory components of the
 * Accelerator.
 *
 * Holds 2D data and a token showing which command from the control unit the
 * data is associated with.
 *
 *  Created on: 1 Jul 2019
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_INTERFACE_H_
#define SRC_TILE_ACCELERATOR_INTERFACE_H_

#include "systemc"
#include "../../Types.h"
#include "AcceleratorTypes.h"

using sc_core::sc_event;
using sc_core::sc_interface;

class accelerator_ifc_base : virtual public sc_interface {
public:
  // The amount of data that can be transmitted simultaneously.
  virtual size2d_t size() const = 0;
};

template<typename T>
class accelerator_producer_ifc : virtual public accelerator_ifc_base {
public:
  // Check whether it is possible to write to the channel at this time.
  virtual bool canWrite() const = 0;

  // Event which is triggered when it becomes possible to write to the channel.
  virtual const sc_event& canWriteEvent() const = 0;

  // Write to the channel, at the given position.
  virtual void write(uint row, uint col, T value) = 0;

  // Notify consumers that all data has now been written. Record which tick of
  // computation this data is associated with.
  virtual void finishedWriting(tick_t tick) = 0;
};

template<typename T>
class accelerator_consumer_ifc : virtual public accelerator_ifc_base {
public:
  // Check whether it is possible to read from the channel at this time.
  virtual bool canRead() const = 0;

  // Event which is triggered when it becomes possible to read from the channel.
  virtual const sc_event& canReadEvent() const = 0;

  // Return which tick of computation this data is associated with.
  virtual tick_t getTick() const = 0;

  // Read from the channel, at the given position. Broadcasting is used to
  // ensure any requested coordinates are valid, even if beyond the bounds of
  // the stored data. A request for row x will access x mod num rows.
  virtual T read(uint row, uint col) const = 0;

  // Notify producers that all data has now been consumed.
  virtual void finishedReading() = 0;
};

template<typename T>
class accelerator_inout_ifc : virtual public accelerator_producer_ifc<T>,
                              virtual public accelerator_consumer_ifc<T> {
  // Nothing extra.
};



#endif /* SRC_TILE_ACCELERATOR_INTERFACE_H_ */
