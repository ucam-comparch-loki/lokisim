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

#include <vector>
#include "../../Datatype/Identifier.h"
#include "../../Typedefs.h"
#include "MemoryTypedefs.h"
#include "CacheLineBuffer.h"

using std::vector;

class AbstractMemoryHandler {
protected:
  //---------------------------------------------------------------------------------------------
  // Configuration parameters
  //---------------------------------------------------------------------------------------------

  uint mSetCount;             // Number of sets in general purpose cache mode
  uint mWayCount;             // Number of ways in general purpose cache mode
  uint mLineSize;             // Size of lines (for cache management and data interleaving)

  const uint cIndexBits;      // Bits required to index a cache line slot (log2(lines in bank))

  //---------------------------------------------------------------------------------------------
  // State
  //---------------------------------------------------------------------------------------------

  uint32_t  *mData;           // Data words stored in the bank

  uint       mLineBits;       // Number of line bits: log2(line size)
  uint32_t   mLineMask;       // Bit mask used to extract line bits

  const uint mBankNumber;     // Bank number for statistics use

  //---------------------------------------------------------------------------------------------
  // Internal functions
  //---------------------------------------------------------------------------------------------

  uint log2Exact(uint value);

public:
  AbstractMemoryHandler(uint bankNumber, vector<uint32_t>& data);
  virtual ~AbstractMemoryHandler();

  // Mappings between memory addresses and SRAM locations.

  // The cache line slot in which this address will be, if stored locally.
  // If modelling an associative cache, returns the first of a set of adjacent lines.
  uint getSlot(MemoryAddr address) const;

  SRAMAddress getLine(SRAMAddress address) const;
  SRAMAddress getOffset(SRAMAddress address) const;

  // Data access operations.

  virtual uint32_t readWord(SRAMAddress address);
  virtual uint32_t readHalfWord(SRAMAddress address);
  virtual uint32_t readByte(SRAMAddress address);

  virtual void writeWord(SRAMAddress address, uint32_t data);
  virtual void writeHalfWord(SRAMAddress address, uint32_t data);
  virtual void writeByte(SRAMAddress address, uint32_t data);

  // Cache line operations.

  virtual CacheLookup lookupCacheLine(MemoryAddr address) const = 0;
  virtual CacheLookup prepareCacheLine(MemoryAddr address, CacheLineBuffer& lineBuffer, bool isRead, bool isInstruction) = 0;
  virtual void replaceCacheLine(CacheLineBuffer& buffer, SRAMAddress position) = 0;
  virtual void fillCacheLineBuffer(MemoryAddr address, CacheLineBuffer& buffer) = 0;
};

#endif /* ABSTRACTMEMORYHANDLER_H_ */
