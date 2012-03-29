/*
 * loki_bus.h
 *
 *  Created on: 28 Mar 2012
 *      Author: db434
 */

#ifndef LOKI_BUS_H_
#define LOKI_BUS_H_

#include "loki_signal.h"
#include "../Network/NetworkTypedefs.h"
#include "../Utility/Instrumentation.h"

class loki_bus : public loki_signal<DataType> {

//==============================//
// Methods
//==============================//

public:

  virtual void ack() {
    outstandingAcks--;

    // We have only finished with this data when all acks have arrived.
    if(outstandingAcks == 0)
      loki_signal<DataType>::ack();
  }

protected:

  // Update the value held in this wire.
  virtual void update() {
    unsigned long cycle = Instrumentation::currentCycle();
    assert(lastWriteTime != cycle);
    lastWriteTime = cycle;

    assert(outstandingAcks == 0);
    outstandingAcks = this->m_new_val.channelID().numDestinations();

    computeSwitching(this->m_cur_val, this->m_new_val);

    loki_signal<DataType>::update();
  }

  void computeSwitching(const DataType& oldval, const DataType& newval) const {
    unsigned int dataDiff = newval.payload().toInt() ^ oldval.payload().toInt();
    unsigned int channelDiff = newval.channelID().getData() ^ oldval.channelID().getData();

    int bitsSwitched = __builtin_popcount(dataDiff) + __builtin_popcount(channelDiff);

    // TODO: figure out how we can associate the number of bits switched with
    // the length of this wire. We probably want to be able to change the
    // length outside of the simulation, so the wire won't know its own length.
    Instrumentation::networkActivity(0, 0, 0, 0, bitsSwitched);
  }

//==============================//
// Constructors and destructors
//==============================//

public:

  loki_bus() : loki_signal<DataType>() {
    lastWriteTime = -1;
    outstandingAcks = 0;
  }

//==============================//
// Local state
//==============================//

private:

  // The cycle when this bus was last written to. Writing more than once in a
  // single cycle is not allowed.
  unsigned long lastWriteTime;

  // If a message goes to multiple destinations, we need to wait for all
  // acknowledgements to arrive before finally sending an acknowledgement.
  unsigned int outstandingAcks;

};

#endif /* LOKI_BUS_H_ */
