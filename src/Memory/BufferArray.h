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
  const T& read(const uint buffer) const {
    return buffers[buffer]->read();
  }

  // Write data to a particular buffer
  void write(const T& data, const uint buffer) {
    buffers[buffer]->write(data);
  }

  // Allows any method of the Buffer to be called
  Buffer<T>& operator[] (const uint index) const {
    return *(buffers[index]);
  }

  uint32_t size() const {
    return buffers.size();
  }

  bool empty() const {
    for(uint i=0; i<size(); i++) {
      if(!buffers[i]->empty()) return false;
    }
    return true;
  }

//==============================//
// Constructors and destructors
//==============================//

public:

  BufferArray(const uint numBuffers, const uint buffSize, std::string name) {
    for(uint i=0; i<numBuffers; i++) {
      std::string buffName = name + ".buffer";
      buffName += i;
      buffers.push_back(new Buffer<T>(buffSize, buffName));
    }
  }

  virtual ~BufferArray() {
    for(uint i=0; i<size(); i++) delete buffers[i];
  }

//==============================//
// Local state
//==============================//

private:

  std::vector<Buffer<T>* > buffers;

};

#endif /* BUFFERARRAY_H_ */
