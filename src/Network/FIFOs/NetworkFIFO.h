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
#include "../../Utility/BlockingInterface.h"
#include "FIFO.h"
#include "../Interface.h"
#include "../../Utility/Instrumentation/Network.h"
#include "../BandwidthMonitor.h"

template<class T>
class NetworkFIFO: public LokiComponent,
                   public network_inout_ifc<T>,
                   public BlockingInterface {

public:
  typedef Flit<T> stored_data;

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput clock;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(NetworkFIFO);
  NetworkFIFO(const sc_module_name& name, size_t size, bandwidth_t bandwidth) :
      LokiComponent(name),
      BlockingInterface(),
      clock("clock"),
      fifo("internal", size),
      fresh(size, false),
      readBandwidth(bandwidth),
      writeBandwidth(bandwidth) {

    SC_METHOD(monitorHeadFlit);
    SC_METHOD(monitorSpaceAvailable);

  }

  NetworkFIFO(const sc_module_name& name, const fifo_parameters_t& params) :
      LokiComponent(name),
      BlockingInterface(),
      clock("clock"),
      fifo("internal", params.size),
      fresh(params.size, false),
      readBandwidth(params.bandwidth),
      writeBandwidth(params.bandwidth) {

    SC_METHOD(monitorHeadFlit);
    SC_METHOD(monitorSpaceAvailable);

  }

  virtual ~NetworkFIFO() {}

//============================================================================//
// Methods
//============================================================================//

public:

  virtual const stored_data read() {
    LOKI_LOG(3) << name() << " consumed " << peek() << endl;
    if (full() && fifo.size() > 1)
      LOKI_LOG(3) << name() << " is no longer full" << endl;
    Instrumentation::Network::recordBandwidth(this->name());

    if (fresh[fifo.getReadPointer()]) {
      dataConsumed.notify();
      fresh[fifo.getReadPointer()] = false;
    }

    readBandwidth.recordEvent();
    return fifo.read();
  }

  virtual const stored_data peek() const {
    return fifo.peek();
  }

  virtual void write(const stored_data& newData) {
    LOKI_LOG(3) << name() << " received " << newData << endl;

    fresh[fifo.getWritePointer()] = true;
    fifo.write(newData);
    writeBandwidth.recordEvent();

    if (full() && fifo.size() > 1)
      LOKI_LOG(3) << name() << " is full" << endl;
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
    return !empty() && readBandwidth.bandwidthAvailable();
  }

  // Event triggered whenever it becomes possible to read from the FIFO.
  virtual const sc_event& canReadEvent() const {
    return newHeadFlit;
  }

  virtual const sc_event& writeEvent() const {
    return fifo.writeEvent();
  }

  virtual bool canWrite() const {
    return !full() && writeBandwidth.bandwidthAvailable();
  }

  // Event triggered whenever it becomes possible to write to the FIFO.
  virtual const sc_event& canWriteEvent() const {
    return newSpaceAvailable;
  }


  unsigned int items() const {
    return fifo.items();
  }

  // Public for IPK FIFO only. Can we avoid exposing this - put in a subclass?
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

protected:

  virtual void reportStalls(ostream& os) {
    if (canRead()) {
      os << this->name() << " has unread data" << endl;
      os << "  First item: " << peek() << endl;
    }
    if (!canWrite())
      os << "  FIFO is full" << endl;
  }

  virtual bool empty() const {
    return fifo.empty();
  }

  virtual bool full() const {
    return fifo.full();
  }

  // Trigger newHeadFlit whenever a new head flit is available for reading.
  void monitorHeadFlit() {
    if (empty())
      next_trigger(writeEvent());
    else if (!readBandwidth.bandwidthAvailable())
      next_trigger(clock.posedge_event());
    // TODO: this won't work when reading multiple flits in one cycle, but is
    // necessary when new data arrives.
    else if (!clock.posedge())
      next_trigger(clock.posedge_event());
    else {
      newHeadFlit.notify(sc_core::SC_ZERO_TIME);
      next_trigger(fifo.readEvent());
    }
  }

  // Trigger newSpaceAvailable whenever a new buffer space is available for
  // writing.
  void monitorSpaceAvailable() {
    if (full())
      next_trigger(fifo.readEvent());
    else if (!writeBandwidth.bandwidthAvailable())
      next_trigger(clock.posedge_event());
    else {
      newSpaceAvailable.notify(sc_core::SC_ZERO_TIME);
      next_trigger(writeEvent());
    }
  }

//============================================================================//
// Local state
//============================================================================//

private:

  // The internal data storage.
  FIFO<stored_data> fifo;

  // One bit to indicate whether each word has been read yet.
  vector<bool> fresh;

  // Event triggered whenever a "fresh" word is read. This should trigger a
  // credit being sent back to the source.
  sc_event dataConsumed;

  // Event triggered whenever the first flit in the queue changes.
  sc_event newHeadFlit;

  // Event triggered whenever a new space is available to be written to.
  sc_event newSpaceAvailable;

  // Record the bandwidth used when reading and writing this FIFO.
  BandwidthMonitor readBandwidth, writeBandwidth;

};

#endif /* NETWORKBUFFER_H_ */
