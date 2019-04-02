/*
 * NetworkFIFO.h
 *
 * Extension of a standard FIFO which includes the ability to detect when
 * data has been consumed, allowing a credit to be sent.
 *
 *  Created on: 29 Oct 2013
 *      Author: db434
 */

#ifndef NETWORKBUFFER_H_
#define NETWORKBUFFER_H_

#include "../../LokiComponent.h"
#include "FIFO.h"
#include "../Interface.h"
#include "../../Utility/Instrumentation/Network.h"

// TODO: add a bandwidth check to ensure there aren't too many reads/writes per
// cycle. BandwidthMonitor class? dataAvailable and canWrite should return false.
template<class T>
class NetworkFIFO: public LokiComponent,
                   public network_inout_ifc<T> {

public:
  typedef Flit<T> stored_data;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  NetworkFIFO(const sc_module_name& name, const size_t size) :
      LokiComponent(name),
      fifo(this->name(), size),
      fresh(size, false) {

  }

  virtual ~NetworkFIFO() {}

//============================================================================//
// Methods
//============================================================================//

public:

  virtual const stored_data read() {
    LOKI_LOG << name() << " consumed " << peek() << endl;
    if (fifo.full() && fifo.size() > 1)
      LOKI_LOG << name() << " is no longer full" << endl;
    Instrumentation::Network::recordBandwidth(this->name());

    if (fresh[fifo.getReadPointer()]) {
      dataConsumed.notify();
      fresh[fifo.getReadPointer()] = false;
    }

    if (items() > 1)
      newHeadFlit.notify(sc_core::SC_ZERO_TIME);

    return fifo.read();
  }

  virtual const stored_data peek() const {
    return fifo.peek();
  }

  virtual void write(const stored_data& newData) {
    LOKI_LOG << name() << " received " << newData << endl;

    if (fifo.empty())
      newHeadFlit.notify(sc_core::SC_ZERO_TIME);

    fresh[fifo.getWritePointer()] = true;
    fifo.write(newData);

    if (fifo.full() && fifo.size() > 1)
      LOKI_LOG << name() << " is full" << endl;
  }

  virtual const stored_data lastDataRead() const {
    // FIXME: this is not valid if the buffer size is 1.
    return fifo.debugRead(fifo.getReadPointer() - 1);
  }

  virtual const stored_data lastDataWritten() const {
    // FIXME: this is not valid if the buffer size is 1.
    return fifo.debugRead(fifo.getWritePointer() - 1);
  }

  const sc_event& dataConsumedEvent() const {
    return dataConsumed;
  }

  virtual bool canRead() const {
    return !fifo.empty();
  }

  virtual const sc_event& canReadEvent() const {
    return newHeadFlit;
  }

  virtual const sc_event& writeEvent() const {
    return fifo.writeEvent();
  }

  virtual bool canWrite() const {
    return !fifo.full();
  }

  virtual const sc_event& canWriteEvent() const {
    return fifo.readEvent();
  }


  unsigned int items() const {
    return fifo.items();
  }

  // Public for IPK FIFO only. Can we avoid exposing this?
  unsigned int getReadPointer() const {
    return fifo.getReadPointer();
  }

  unsigned int getWritePointer() const {
    return fifo.getWritePointer();
  }

  void setReadPointer(unsigned int position) {
    fifo.setReadPointer(position);
  }

  void setWritePointer(unsigned int position) {
    fifo.setWritePointer(position);
  }

//============================================================================//
// Local state
//============================================================================//

protected:

  // The internal data storage.
  FIFO<stored_data> fifo;

  // One bit to indicate whether each word has been read yet.
  vector<bool> fresh;

  // Event triggered whenever a "fresh" word is read. This should trigger a
  // credit being sent back to the source.
  sc_event dataConsumed;

  // Event triggered whenever the first flit in the queue changes.
  sc_event newHeadFlit;

};

#endif /* NETWORKBUFFER_H_ */
