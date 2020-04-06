/*
 * Interconnect.h
 *
 * Interfaces used to connect PEs and memories.
 *
 * The paper includes an "enable" signal. We ignore that here and instead use
 * SystemC's event system to notify when new data arrives.
 *
 *  Created on: Feb 13, 2020
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_EYERISS_INTERCONNECT_H_
#define SRC_TILE_ACCELERATOR_EYERISS_INTERCONNECT_H_

#include "../../../LokiComponent.h"

using sc_core::sc_event;
using sc_core::sc_interface;

namespace Eyeriss {

template<typename T>
class interconnect_read_ifc : virtual public sc_interface {

public:

  // Event triggered when data becomes available.
  virtual const sc_event& default_event() const = 0;

  // Read data.
  virtual const T read() const = 0;

  // Flow control.
  virtual void stall() = 0;
  virtual void unstall() = 0;

};

template<typename T>
class interconnect_write_ifc : virtual public sc_interface {

public:

  // Event triggered when any output blocks further data from arriving. If the
  // interconnect is already stalled, the event does not trigger.
  virtual const sc_event& stallEvent() const = 0;

  // Event triggered when all recipients are ready for new data.
  virtual const sc_event& unstallEvent() const = 0;

  // Write data.
  virtual void write(T data) = 0;

  // Flow control.
  virtual bool ready() const = 0;

};

template<typename T>
class MulticastBus : public LokiComponent,
                     virtual public interconnect_read_ifc<T>,
                     virtual public interconnect_write_ifc<T> {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  MulticastBus(sc_module_name name);

//============================================================================//
// Methods
//============================================================================//

public:

  // Event triggered when data becomes available.
  virtual const sc_event& default_event() const;

  // Read data.
  virtual const T read() const;

  // Flow control.
  virtual void stall();
  virtual void unstall();


  // Event triggered when any output blocks further data from arriving. If the
  // interconnect is already stalled, the event does not trigger.
  virtual const sc_event& stallEvent() const;

  // Event triggered when all recipients are ready for new data.
  virtual const sc_event& unstallEvent() const;

  // Write data.
  virtual void write(T data);

  // Flow control.
  virtual bool ready() const;

//============================================================================//
// Local state
//============================================================================//

private:

  T data;

  // The number of outputs which are not ready to receive new data.
  uint numStalled;

  sc_event newDataEvent;
  sc_event stalledEvent;
  sc_event unstalledEvent;

};

} /* namespace Eyeriss */

#endif /* SRC_TILE_ACCELERATOR_EYERISS_INTERCONNECT_H_ */
