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

template<class T>
class NetworkFIFO: public FIFO<T> {

//============================================================================//
// Methods
//============================================================================//

public:

  virtual const T& read() {
    if (this->full())
      LOKI_LOG << this->name() << " is no longer full" << endl;

    if (fresh[this->readPos.value()]) {
      dataConsumed.notify();
      fresh[this->readPos.value()] = false;
    }
    return FIFO<T>::read();
  }

  virtual void write(const T& newData) {
    fresh[this->writePos.value()] = true;
    FIFO<T>::write(newData);

    if (this->full())
      LOKI_LOG << this->name() << " is full" << endl;
  }

  const sc_event& dataConsumedEvent() const {
    return dataConsumed;
  }

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  NetworkFIFO(const std::string& name, const size_t size) :
      FIFO<T>(size, name),
      fresh(size, false) {

  }

  virtual ~NetworkFIFO() {}

//============================================================================//
// Local state
//============================================================================//

protected:

  // One bit to indicate whether each word has been read yet.
  vector<bool> fresh;

  // Event triggered whenever a "fresh" word is read. This should trigger a
  // credit being sent back to the source.
  sc_event dataConsumed;

};

#endif /* NETWORKBUFFER_H_ */
