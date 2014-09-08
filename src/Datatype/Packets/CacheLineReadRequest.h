/*
 * CacheLineReadRequest.h
 *
 * A request for a cache line from a particular memory address.
 *
 * A network address must be provided so that the cache line can be returned
 * to the memory bank which asked for it.
 *
 * The request consists of two flits:
 *  1. The address of the first word to be accessed.
 *  2. The network address of the component to send the cache line back to.
 *
 *  Created on: 3 Sep 2014
 *      Author: db434
 */

#ifndef CACHELINEREADREQUEST_H_
#define CACHELINEREADREQUEST_H_

#include "Packet.h"
#include "../../Datatype/Word.h"

class CacheLineReadRequest: public Packet<Word> {

private:

  static const uint MEMORY_REQUEST_FLIT = 0;
  static const uint RETURN_ADDRESS_FLIT = 1;

//==============================//
// Constructors
//==============================//

public:

  CacheLineReadRequest() : Packet<Word>() {

  }

  CacheLineReadRequest(MemoryAddr memAddr,      // The first word of the cache line
                       size_t     lineSize,     // Cache line length in bytes
                       ChannelID  destination,  // The component to read the line from
                       ChannelID  source)       // The component to return the line to
      : Packet<Word>() {

    MemoryRequest request(MemoryRequest::FETCH_LINE, memAddr, lineSize);
    Flit<Word> requestFlit(request, destination);
    requestFlit.setEndOfPacket(false);
    addFlit(requestFlit);

    Flit<Word> returnAddressFlit(source, destination);
    returnAddressFlit.setEndOfPacket(true);
    addFlit(returnAddressFlit);

  }

//==============================//
// Methods
//==============================//

public:

  MemoryAddr memoryAddress() const {
    assert(haveMemoryAddress());
    MemoryRequest request = static_cast<MemoryRequest>(getFlit(MEMORY_REQUEST_FLIT).payload());
    return request.getPayload();
  }

  bool haveMemoryAddress() const {
    return flitsReceived() > MEMORY_REQUEST_FLIT;
  }

  size_t lineSize() const {
    assert(haveMemoryAddress());
    MemoryRequest request = static_cast<MemoryRequest>(getFlit(MEMORY_REQUEST_FLIT).payload());
    return request.getLineSize();
  }

  bool haveLineSize() const {
    return flitsReceived() > MEMORY_REQUEST_FLIT;
  }

  ChannelID returnAddress() const {
    assert(haveReturnAddress());
    return static_cast<ChannelID>(getFlit(RETURN_ADDRESS_FLIT).payload());
  }

  bool haveReturnAddress() const {
    return flitsReceived() >= RETURN_ADDRESS_FLIT;
  }

};

#endif /* CACHELINEREADREQUEST_H_ */
