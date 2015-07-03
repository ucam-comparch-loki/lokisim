/*
 * Directory.h
 *
 * L1 cache directory. Used to redirect cache accesses to other L1s on a cache
 * miss. This can be used to group multiple tiles' caches together.
 *
 * On a cache miss, a few bits are taken from the address and used to index
 * into the directory. The directory then returns the "home tile" for that data
 * so the request can be redirected. If this cache is on the home tile, the
 * request must be sent to the next level of the cache hierarchy.
 *
 *  Created on: 24 Jun 2014
 *      Author: db434
 */

#ifndef DIRECTORY_H_
#define DIRECTORY_H_

#include <vector>

#include "../../Datatype/Identifier.h"
#include "../../Typedefs.h"
#include "MemoryTypedefs.h"

class Directory {

//==============================//
// Constructors and destructors
//==============================//

public:

  Directory(unsigned int size);

//==============================//
// Methods
//==============================//

public:

  // Set the bitmask which determines which bits of the address should be used
  // to access the directory. Only the least significant bit is needed - the
  // remaining bits are determined according to the size of the directory.
  void setBitmaskLSB(unsigned int lsb);

  // Set directory[entry] = tile.
  void setEntry(unsigned int entry, TileIndex tile);

  // Set all entries to point to a particular tile. This is useful if the L1
  // sharing isn't going to be used as it allows all requests to be forwarded
  // to the next cache level without requiring that the directory is set up
  // first.
  void initialise(TileIndex tile);

  // Return the tile responsible for caching this memory address.
  TileID getTile(MemoryAddr address) const;

//==============================//
// Local state
//==============================//

private:

  const unsigned int bitmask;
  unsigned int shiftAmount;

  std::vector<TileIndex> directory;

};

#endif /* DIRECTORY_H_ */
