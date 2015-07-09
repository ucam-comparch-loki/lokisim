/*
 * MemoryRequest.h
 *
 * Request to set up a channel with a memory. Specify the type of operation
 * to be performed and the address in memory to perform it.
 *
 *  Created on: 3 Mar 2010
 *      Author: db434
 */

#ifndef MEMORYREQUEST_H_
#define MEMORYREQUEST_H_

#include "Word.h"
#include "Identifier.h"
#include "../Exceptions/InvalidOptionException.h"
#include "../Typedefs.h"
#include "../TileComponents/Memory/MemoryTypedefs.h"

class MemoryRequest : public Word {
private:
  // | Address : 32                                                          |
  // | Unused : 22                       | TileX : 3 | TileY : 3 | Entry : 4 |

  static const uint OFFSET_TILE             = 4;
  static const uint  WIDTH_TILE             = 6;
  static const uint OFFSET_DIRECTORY_ENTRY  = 0;
  static const uint  WIDTH_DIRECTORY_ENTRY  = 4;

public:

  inline uint32_t        getPayload()    const {return data_ & 0xFFFFFFFFULL;}

  inline uint            getDirectoryEntry() const {return getBits(OFFSET_DIRECTORY_ENTRY, OFFSET_DIRECTORY_ENTRY + WIDTH_DIRECTORY_ENTRY - 1);}
  inline TileIndex       getTile()       const {return getBits(OFFSET_TILE, OFFSET_TILE + WIDTH_TILE - 1);}

  MemoryRequest() : Word() {
    // Nothing
  }

  MemoryRequest(const Word& other) : Word(other) {
    // Nothing
  }

};

#endif /* MEMORYREQUEST_H_ */
