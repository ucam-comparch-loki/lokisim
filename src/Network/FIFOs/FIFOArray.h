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

#include "../../LokiComponent.h"
#include "../../Utility/LokiVector.h"
#include "NetworkFIFO.h"

template<class T>
class FIFOArray: public LokiComponent {

//============================================================================//
// Methods
//============================================================================//

public:

  // Allows any method of the Buffer to be called
  NetworkFIFO<T>& operator[] (const uint index) const {
    return buffers[index];
  }

  uint32_t size() const {
    return buffers.size();
  }

  bool empty() const {
    for(uint i=0; i<size(); i++) {
      if(!buffers[i].empty()) return false;
    }
    return true;
  }

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  FIFOArray(const sc_module_name& name, const uint numBuffers, const uint buffSize) :
      LokiComponent(name) {
    assert(numBuffers > 0);
    assert(buffSize > 0);

    for(uint i=0; i<numBuffers; i++) {
      std::stringstream ss;
      ss << i;
      buffers.push_back(new NetworkFIFO<T>(ss.str().c_str(), buffSize));
    }
  }

//============================================================================//
// Local state
//============================================================================//

private:

  LokiVector<NetworkFIFO<T>> buffers;

};

#endif /* BUFFERARRAY_H_ */
