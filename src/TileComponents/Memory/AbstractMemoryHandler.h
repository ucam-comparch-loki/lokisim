/*
 * AbstractMemoryHandler.h
 *
 * A common parent class for the GeneralPurposeCacheHandler and
 * ScratchpadModeHandler.
 *
 *  Created on: 8 Jan 2015
 *      Author: db434
 */

#ifndef ABSTRACTMEMORYHANDLER_H_
#define ABSTRACTMEMORYHANDLER_H_

#include "../../Typedefs.h"
#include "MemoryTypedefs.h"

class AbstractMemoryHandler {
protected:
  //---------------------------------------------------------------------------------------------
  // Configuration parameters
  //---------------------------------------------------------------------------------------------

  uint mSetCount;             // Number of sets in general purpose cache mode
  uint mWayCount;             // Number of ways in general purpose cache mode
  uint mLineSize;             // Size of lines (for cache management and data interleaving)

  //---------------------------------------------------------------------------------------------
  // State
  //---------------------------------------------------------------------------------------------

  uint32_t  *mData;           // Data words stored in the bank

  uint       mLineBits;       // Number of line bits - log2(line size)
  uint32_t   mLineMask;       // Bit mask used to extract line bits

  uint       mGroupIndex;     // Relative index of bank within group (off by one)
  uint       mGroupBits;      // Number of group bits - log2(group size)
  uint32_t   mGroupMask;      // Bit mask used to extract group bits

  const uint mBankNumber;     // Bank number for statistics use

  //---------------------------------------------------------------------------------------------
  // Internal functions
  //---------------------------------------------------------------------------------------------

  uint log2Exact(uint value);

public:
  AbstractMemoryHandler(uint bankNumber);
  virtual ~AbstractMemoryHandler();

  // Set up the memory bank with a new configuration. This invalidates any data
  // currently stored.
  virtual void activate(const MemoryConfig& config) = 0;

  // Check whether this memory bank is responsible for storing data at the
  // given address, according to the current mapping of addresses to banks.
  // Note that "true" does not mean that the data is present.
  virtual bool containsAddress(uint32_t address) = 0;

  // Check whether two memory addresses map to the same cache line.
  virtual bool sameLine(uint32_t address1, uint32_t address2) = 0;

  // Would also like to get the read/write methods in here.
  // Currently, the cache also returns whether there was a hit or not.
};

#endif /* ABSTRACTMEMORYHANDLER_H_ */
