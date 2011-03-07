/*
 * BufferWithCounter.h
 *
 * A buffer with a credit counter.
 *
 *  Created on: 4 Mar 2011
 *      Author: db434
 */

#ifndef BUFFERWITHCOUNTER_H_
#define BUFFERWITHCOUNTER_H_

#include "Buffer.h"

template<class T>
class BufferWithCounter : public Buffer<T> {

//==============================//
// Ports
//==============================//

// Inherited from Buffer:
//   sc_in<T>      dataIn;
//   sc_in<bool>   doRead;
//   sc_out<T>     dataOut;

public:

  sc_in<bool>  credits;
  sc_out<bool> emptySig;          // Should these ports
  sc_out<bool> flowControlOut;    // also be in Buffer?

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(BufferWithCounter);

  BufferWithCounter(sc_module_name name, ComponentID ID, uint size) :
    Buffer<T>(name, ID, size) {
    creditCount = CHANNEL_END_BUFFER_SIZE;

    SC_METHOD(receivedCredit);
    this->sensitive << credits;
    this->dont_initialize();

    SC_METHOD(updateFullEmpty);
    this->sensitive << fullnessChanged;
    this->dont_initialize();
  }

//==============================//
// Methods
//==============================//

public:

  bool canSend() const {
    return !this->empty() && creditCount > 0;
  }

protected:

  virtual void read() {
    // What should happen if the buffer is empty?
    //  1. Disallow reads (have an "empty" output signal).
    //  2. Wait until new data arrives.
    assert(canSend());
    this->dataOut.write(this->buffer.read());
    creditCount--;
    fullnessChanged.notify(sc_core::SC_ZERO_TIME);
  }

  virtual void write() {
    assert(!this->full());
    this->buffer.write(this->dataIn.read());
    fullnessChanged.notify(sc_core::SC_ZERO_TIME);
  }

  void receivedCredit() {
    creditCount++;
    fullnessChanged.notify(sc_core::SC_ZERO_TIME);
  }

  void updateFullEmpty() {
    assert(creditCount >= 0 && creditCount <= CHANNEL_END_BUFFER_SIZE);

    // Have a slightly extended definition of emptiness so readers of the signal
    // can know that it is safe to perform some operations.
    emptySig.write(this->empty() && creditCount >= CHANNEL_END_BUFFER_SIZE);

    flowControlOut.write(!this->full());
  }

//==============================//
// Local state
//==============================//

private:

  unsigned int creditCount;

  sc_core::sc_event fullnessChanged;

};


#endif /* BUFFERWITHCOUNTER_H_ */
