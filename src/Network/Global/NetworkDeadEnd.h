/*
 * NetworkDeadEnd.h
 *
 * Dummy component to be attached to the edge of the network. Warnings are
 * displayed if any data is received or requested.
 *
 *  Created on: 6 Oct 2016
 *      Author: db434
 */

#ifndef SRC_NETWORK_GLOBAL_NETWORKDEADEND_H_
#define SRC_NETWORK_GLOBAL_NETWORKDEADEND_H_

#include "../../LokiComponent.h"
#include "../Interface.h"
#include "../../Datatype/Flit.h"

template<typename T>
class NetworkDeadEnd: public LokiComponent,
                      public network_source_ifc<T>,
                      public network_sink_ifc<T> {

public:

  NetworkDeadEnd(const sc_module_name& name, TileID id, string direction) :
      LokiComponent(name),
      id(id),
      direction(direction) {

  }

  virtual const Flit<T> read() {
    LOKI_WARN << "Trying to read from " << direction << " of tile " << id << endl;
    return dummyFlit;
  }

  virtual const Flit<T> peek() const {
    LOKI_WARN << "Trying to peek from " << direction << " of tile " << id << endl;
    return dummyFlit;
  }

  virtual bool canRead() const {
    return false;
  }

  virtual const sc_event& canReadEvent() const {
    return dummyEvent;
  }

  virtual const sc_event& writeEvent() const {
    return dummyEvent;
  }

  virtual const Flit<T> lastDataRead() const {
    LOKI_WARN << "Trying to debug read from " << direction << " of tile " << id << endl;
    return dummyFlit;
  }

  virtual void write(const Flit<T>& data) {
    LOKI_WARN << "Trying to write to " << direction << " of tile " << id << endl;
    LOKI_WARN << "  Data: " << data << endl;
  }

  virtual bool canWrite() const {
    // Allow the data to arrive so we can print a useful message.
    return true;
  }

  virtual const sc_event& canWriteEvent() const {
    return dummyEvent;
  }

  virtual const sc_event& dataConsumedEvent() const {
    return dummyEvent;
  }

  virtual const Flit<T> lastDataWritten() const {
    LOKI_WARN << "Trying to debug read from " << direction << " of tile " << id << endl;
    return dummyFlit;
  }

private:

  const TileID id;

  // The direction data is being sent to arrive at this dead end.
  const string direction;

  // Event which is never triggered - nothing ever happens here.
  sc_event dummyEvent;

  Flit<T> dummyFlit;

};

#endif /* SRC_NETWORK_GLOBAL_NETWORKDEADEND_H_ */
