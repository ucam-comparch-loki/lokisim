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

  // Data access operations. Return value is whether the access was a hit.
  // Any data to be returned is passed through the reference &data.
  //   instruction = is this an instruction access? (For statistics only.)
  //   resume      = is this a resumed operation after a cache miss? (Cache mode only.)
  //   debug       = should this operation be logged in the execution statistics?

  virtual bool readWord(MemoryAddr address, uint32_t &data, bool instruction, bool resume, bool debug, int core, int retCh) = 0;
  virtual bool readHalfWord(MemoryAddr address, uint32_t &data, bool resume, bool debug, int core, int retCh) = 0;
  virtual bool readByte(MemoryAddr address, uint32_t &data, bool resume, bool debug, int core, int retCh) = 0;

  virtual bool writeWord(MemoryAddr address, uint32_t data, bool resume, bool debug, int core, int retCh) = 0;
  virtual bool writeHalfWord(MemoryAddr address, uint32_t data, bool resume, bool debug, int core, int retCh) = 0;
  virtual bool writeByte(MemoryAddr address, uint32_t data, bool resume, bool debug, int core, int retCh) = 0;
};

#endif /* ABSTRACTMEMORYHANDLER_H_ */
