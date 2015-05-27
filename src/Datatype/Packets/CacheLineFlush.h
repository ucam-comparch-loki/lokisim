/*
 * CacheLineFlush.h
 *
 * A cache line which is being flushed back to the next level of the
 * memory hierarchy.
 *
 * The packet consists of two parts:
 *  1. A request flit outlining where the data should go and how much of it
 *     there is.
 *  2. The cache line.
 *
 *  Created on: 3 Sep 2014
 *      Author: db434
 */

#ifndef CACHELINEFLUSH_H_
#define CACHELINEFLUSH_H_

#include "Packet.h"
#include "../../Datatype/Word.h"

class CacheLineFlush: public Packet<Word> {

private:

  static const uint MEMORY_REQUEST_FLIT = 0;
  static const uint START_DATA_FLIT = 1;

//==============================//
// Constructors
//==============================//

public:

  CacheLineFlush() : Packet<Word>() {

  }

  CacheLineFlush(MemoryAddr   memAddr,      // The first word of the cache line
                 size_t       lineSize,     // Cache line length in bytes
                 uint32_t    *data,         // Cache line
                 ChannelID    destination)  // The component to write the data to
      : Packet<Word>() {

    MemoryRequest request(MemoryRequest::STORE_LINE, memAddr, lineSize, destination.component.tile, destination.component.position);
    Flit<Word> requestFlit(request, destination);
    requestFlit.setEndOfPacket(false);
    addFlit(requestFlit);

    size_t words = lineSize/4;

    for (uint i=0; i<words; i++) {
      Flit<Word> dataFlit(data[i], destination);
      dataFlit.setEndOfPacket(i == words-1);
      addFlit(dataFlit);
    }

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
    assert(haveLineSize());
    MemoryRequest request = static_cast<MemoryRequest>(getFlit(MEMORY_REQUEST_FLIT).payload());
    return request.getLineSize();
  }

  bool haveLineSize() const {
    return flitsReceived() > MEMORY_REQUEST_FLIT;
  }

  Word data(int index) {
    assert(haveData(index));
    return getFlit(START_DATA_FLIT + index).payload();
  }

  bool haveData(int index) {
    return flitsReceived() > START_DATA_FLIT + index;
  }

};

#endif /* CACHELINEFLUSH_H_ */
