/*
 * NetworkBuffer.h
 *
 * Extension of a standard buffer which includes the ability to detect when
 * data has been consumed, allowing a credit to be sent.
 *
 *  Created on: 29 Oct 2013
 *      Author: db434
 */

#ifndef NETWORKBUFFER_H_
#define NETWORKBUFFER_H_

#include "../Memory/BufferStorage.h"

template<class T>
class NetworkBuffer: public BufferStorage<T> {

//==============================//
// Methods
//==============================//

public:

  virtual const T& read() {
    if (DEBUG && this->full())
      cout << this->name() << " is no longer full" << endl;

    if (fresh[this->readPos.value()]) {
      dataConsumed.notify();
      fresh[this->readPos.value()] = false;
    }
    return BufferStorage<T>::read();
  }

  virtual void write(const T& newData) {
    fresh[this->writePos.value()] = true;
    BufferStorage<T>::write(newData);

    if (DEBUG && this->full())
      cout << this->name() << " is full" << endl;
  }

  const sc_event& dataConsumedEvent() const {
    return dataConsumed;
  }

//==============================//
// Constructors and destructors
//==============================//

public:

  NetworkBuffer(const std::string& name, const size_t size) :
      BufferStorage<T>(size, name),
      fresh(size, false) {

  }

  virtual ~NetworkBuffer() {}

//==============================//
// Local state
//==============================//

protected:

  // One bit to indicate whether each word has been read yet.
  vector<bool> fresh;

  // Event triggered whenever a "fresh" word is read. This should trigger a
  // credit being sent back to the source.
  sc_event dataConsumed;

};

#endif /* NETWORKBUFFER_H_ */
