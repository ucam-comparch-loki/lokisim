/*
 * Buffer.h
 *
 * A buffer with SystemC ports.
 *
 *  Created on: 2 Mar 2011
 *      Author: db434
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#include "../Component.h"
#include "../Memory/BufferStorage.h"

template<class T>
class Buffer : public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<T>      dataIn;
  sc_in<bool>   doRead;
  sc_out<T>     dataOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Buffer);

  Buffer(sc_module_name name, ComponentID ID, uint size) :
    Component(name, ID),
    buffer(size, this->name()) {

    SC_METHOD(read);
    sensitive << doRead;
    dont_initialize();

    SC_METHOD(write);
    sensitive << dataIn;
    dont_initialize();

  }

//==============================//
// Methods
//==============================//

public:

  bool empty() const {
    return buffer.empty();
  }

  bool full() const {
    return buffer.full();
  }

  const T& peek() const {
    assert(!empty());
    return buffer.peek();
  }

protected:

  virtual void read() {
    // What should happen if the buffer is empty?
    //  1. Disallow reads (have an "empty" output signal).
    //  2. Wait until new data arrives.
    assert(!empty());
    dataOut.write(buffer.read());
  }

  virtual void write() {
    assert(!full());
    buffer.write(dataIn.read());
  }

//==============================//
// Local state
//==============================//

protected:

  BufferStorage<T> buffer;

};

#endif /* BUFFER_H_ */
