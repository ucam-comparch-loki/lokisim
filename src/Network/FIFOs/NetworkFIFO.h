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

#include "FIFO.h"
#include "../Interface.h"
#include "../../Utility/Instrumentation/Network.h"

template<class T>
class NetworkFIFO: public network_source_ifc<T>,
                   public network_sink_ifc<T> {

  typedef Flit<T> stored_data;

//============================================================================//
// Methods
//============================================================================//

public:

  virtual const stored_data& read() {
    LOKI_LOG << name() << " consumed " << peek() << endl;
    if (fifo.full())
      LOKI_LOG << name() << " is no longer full" << endl;
    Instrumentation::Network::recordBandwidth(this->name().c_str());

    if (fresh[fifo.getReadPointer()]) {
      dataConsumed.notify();
      fresh[fifo.getReadPointer()] = false;
    }
    return fifo.read();
  }

  virtual const stored_data& peek() const {
    return fifo.peek();
  }

  virtual void write(const stored_data& newData) {
    LOKI_LOG << name() << " received " << newData << endl;

    fresh[fifo.getWritePointer()] = true;
    fifo.write(newData);

    if (fifo.full())
      LOKI_LOG << fifo.name() << " is full" << endl;
  }

  virtual const stored_data& lastDataRead() const {
    return fifo.debugRead(fifo.getReadPointer() - 1);
  }

  virtual const stored_data& lastDataWritten() const {
    return fifo.debugRead(fifo.getWritePointer() - 1);
  }

  const sc_event& dataConsumedEvent() const {
    return dataConsumed;
  }

  virtual bool dataAvailable() const {
    return !fifo.empty();
  }

  virtual const sc_event& dataAvailableEvent() const {
    // Should this also be triggered when reading, if there is more data already
    // waiting? No - there are some cases where we only care about the writing.
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

  const std::string name() const {
    return fifo.name();
  }

  // For IPK FIFO only. Can we avoid exposing this?
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
// Constructors and destructors
//============================================================================//

public:

  NetworkFIFO(const std::string& name, const size_t size) :
      fifo(name, size),
      fresh(size, false) {

  }

  virtual ~NetworkFIFO() {}

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

};

#endif /* NETWORKBUFFER_H_ */
