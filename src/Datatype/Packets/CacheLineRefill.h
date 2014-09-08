/*
 * CacheLineRefill.h
 *
 * A cache line which has been specifically requested. No extra information
 * about the cache line (e.g. memory address) is required since the memory
 * bank will be stalled waiting for it to arrive.
 *
 *  Created on: 3 Sep 2014
 *      Author: db434
 */

#ifndef CACHELINEREFILL_H_
#define CACHELINEREFILL_H_

#include "Packet.h"
#include "../../Datatype/Word.h"

class CacheLineRefill: public Packet<Word> {

private:

  static const uint START_DATA_FLIT = 0;

//==============================//
// Constructors
//==============================//

public:

  CacheLineRefill() : Packet<Word>() {

  }

  CacheLineRefill(uint32_t    *data,          // Cache line
                  size_t       lineSize,      // Cache line size in bytes
                  ChannelID    destination)   // The component to write the data to
      : Packet<Word>() {

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

  Word data(int index) {
    assert(haveData(index));
    return getFlit(START_DATA_FLIT + index).payload();
  }

  bool haveData(int index) {
    return flitsReceived() > START_DATA_FLIT + index;
  }

};

#endif /* CACHELINEREFILL_H_ */
