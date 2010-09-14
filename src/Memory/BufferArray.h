/*
 * BufferArray.h
 *
 * Structure representing an array of buffers, allowing easy and consistent
 * access to individual buffers.
 *
 * Since this class is templated, all of the implementation must go in the
 * header file.
 *
 *  Created on: 10 Mar 2010
 *      Author: db434
 */

#ifndef BUFFERARRAY_H_
#define BUFFERARRAY_H_

#include "Buffer.h"

template<class T>
class BufferArray {

//==============================//
// Methods
//==============================//

public:

  // Read data from a particular buffer
  T& read(int buffer) const {
    return buffers[buffer]->read();
  }

  // Write data to a particular buffer
  void write(const T& data, int buffer) {
    buffers[buffer]->write(data);
  }

  // Allows any method of the Buffer to be called
  Buffer<T>& operator[] (int index) {
    return *(buffers[index]);
  }

  uint32_t size() {
    return buffers.size();
  }

//==============================//
// Constructors and destructors
//==============================//

public:

  BufferArray(int numBuffers, int buffSize) {
    for(int i=0; i<numBuffers; i++) {
      buffers.push_back(new Buffer<T>(buffSize));
    }
  }

//==============================//
// Local state
//==============================//

private:

  std::vector<Buffer<T>* > buffers;

};

#endif /* BUFFERARRAY_H_ */
