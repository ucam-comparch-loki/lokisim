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
 * 2015/09/10: The directory also holds some extra bits which replace the index
 *             bits of the address for the next level of memory hierarchy.
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

private:

  struct DirectoryEntry {
    uint      replaceBits : 4; // Replacements for the address's index bits
    TileID    tile;            // The tile responsible for holding an address

    DirectoryEntry() :
        replaceBits(0),
        tile(0) {
      // Nothing
    }

    DirectoryEntry(uint flattened) :
        replaceBits((flattened >> 6) & 0xF),
        tile((flattened >> 0) & 0x3F) {
      // Nothing
    }

    uint flatten() {
      return (replaceBits << 6) | (tile.flatten() << 0);
    }
  }  __attribute__((packed));

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  Directory(unsigned int size);

//============================================================================//
// Methods
//============================================================================//

public:

  // Set the bitmask which determines which bits of the address should be used
  // to access the directory. Only the least significant bit is needed - the
  // remaining bits are determined according to the size of the directory.
  void setBitmaskLSB(unsigned int lsb);

  // Set directory[entry] = data.
  void setEntry(unsigned int entry, unsigned int data);

  // Set all entries to point to a particular tile. This is useful if the L1
  // sharing isn't going to be used as it allows all requests to be forwarded
  // to the next cache level without requiring that the directory is set up
  // first.
  void initialise(TileID tile);

  // Determine which entry of the directory to access with the given memory
  // address.
  unsigned int getEntry(MemoryAddr address) const;

  // Return the tile responsible for caching this memory address.
  TileID getTile(MemoryAddr address) const;

  // Replace the index bits of the address according to the directory contents.
  MemoryAddr updateAddress(MemoryAddr address) const;

//============================================================================//
// Local state
//============================================================================//

private:

  // Mask to extract index bits from an address. Address must first be shifted
  // right by shiftAmount.
  const unsigned int bitmask;

  // Location of first index bit in a memory address.
  unsigned int shiftAmount;

  // Data array.
  std::vector<DirectoryEntry> directory;

};

#endif /* DIRECTORY_H_ */
