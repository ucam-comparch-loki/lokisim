/*
 * AbstractMemoryHandler.cpp
 *
 *  Created on: 8 Jan 2015
 *      Author: db434
 */

#include "AbstractMemoryHandler.h"
#include "../../Utility/Parameters.h"
#include "../../Utility/Warnings.h"

#include <cassert>
#include <vector>
#include "../../Exceptions/UnalignedAccessException.h"

using std::vector;

uint AbstractMemoryHandler::log2Exact(uint value) {
  assert(value > 1);

  uint result = 0;
  uint temp = value >> 1;

  while (temp != 0) {
    result++;
    temp >>= 1;
  }

  assert(1UL << result == value);

  return result;
}

AbstractMemoryHandler::AbstractMemoryHandler(uint bankNumber, vector<uint32_t>& data) :
    cCacheLines(MEMORY_BANK_SIZE / (CACHE_LINE_WORDS * BYTES_PER_WORD)),
    cIndexBits(log2Exact(cCacheLines)),
    mBankNumber(bankNumber) {

  //-- Configuration parameters -----------------------------------------------------------------

  mSetCount = -1;
  mWayCount = -1;
  mLineSize = -1;

  //-- State ------------------------------------------------------------------------------------

  mData = data.data();

  mLineBits = -1;
  mLineMask = 0;

}

AbstractMemoryHandler::~AbstractMemoryHandler() {
}

uint AbstractMemoryHandler::getSlot(MemoryAddr address) const {
  // Memory address contains:
  // | tag | index | bank | offset |
  //  * offset = 5 bits (32 byte cache line)
  //  * index + offset = log2(bytes in bank) bits
  //  * bank = up to 3 bits used to choose which bank to access
  //  * tag = any bits remaining
  //
  // Since the number of bank bits is variable, but we don't want to move the
  // index bits around or change the size of the tag field, we hash the maximum
  // number of bank bits in with a fixed-position index field. Note that these
  // overlapping bits must now also be included in the tag.
  //
  // Note that this can result in a predictable but counter-intuitive mapping
  // of addresses in scratchpad mode.

  uint bank = (address >> 5) & 0x7;
  uint index = (address >> 8) & ((1 << cIndexBits) - 1);
  uint slot = index ^ (bank << (cIndexBits - 3)); // Hash bank into the upper bits
  return slot;
}

SRAMAddress AbstractMemoryHandler::getLine(SRAMAddress address) const {
  // Cache line = 32 bytes = 2^5 bytes.
  return address >> 5;
}

SRAMAddress AbstractMemoryHandler::getOffset(SRAMAddress address) const {
  // Cache line = 32 bytes = 2^5 bytes.
  return (address & 0x1F);
}

uint32_t AbstractMemoryHandler::readWord(SRAMAddress address) {
  if (WARN_UNALIGNED && (address & 0x3) != 0)
    std::cerr << "Warning: Attempting to access address 0x" << std::hex << address
        << std::dec << " with alignment 4." << std::endl;

  return mData[address/4];
}

uint32_t AbstractMemoryHandler::readHalfWord(SRAMAddress address) {
  if (WARN_UNALIGNED && (address & 0x1) != 0)
    std::cerr << "Warning: Attempting to access address 0x" << std::hex << address
        << std::dec << " with alignment 2." << std::endl;

  uint32_t fullWord = readWord(address & ~0x3);
  return ((address & 0x3) == 0) ? (fullWord & 0xFFFFUL) : (fullWord >> 16); // Little endian
}

uint32_t AbstractMemoryHandler::readByte(SRAMAddress address) {
  uint32_t fullWord = readWord(address & ~0x3);
  uint32_t selector = address & 0x3UL;

  switch (selector) { // Little endian
    case 0: return  fullWord        & 0xFFUL; break;
    case 1: return (fullWord >> 8)  & 0xFFUL; break;
    case 2: return (fullWord >> 16) & 0xFFUL; break;
    case 3: return (fullWord >> 24) & 0xFFUL; break;
    default: assert(false); return 0; break;
  }
}


void AbstractMemoryHandler::writeWord(SRAMAddress address, uint32_t data) {
  if (WARN_UNALIGNED && (address & 0x3) != 0)
    std::cerr << "Warning: Attempting to access address 0x" << std::hex << address
        << std::dec << " with alignment 4." << std::endl;

  mData[address/4] = data;
}

void AbstractMemoryHandler::writeHalfWord(SRAMAddress address, uint32_t data) {
  if (WARN_UNALIGNED && (address & 0x1) != 0)
    std::cerr << "Warning: Attempting to access address 0x" << std::hex << address
        << std::dec << " with alignment 2." << std::endl;

  uint32_t oldData = readWord(address & ~0x3);
  uint32_t newData;

  if ((address & 0x3) == 0) // Little endian
    newData = (oldData & 0xFFFF0000UL) | (data & 0x0000FFFFUL);
  else
    newData = (oldData & 0x0000FFFFUL) | (data << 16);

  writeWord(address & ~0x3, newData);
}

void AbstractMemoryHandler::writeByte(SRAMAddress address, uint32_t data) {
  uint32_t oldData = readWord(address & ~0x3);
  uint32_t selector = address & 0x3UL;
  uint32_t newData;

  switch (selector) { // Little endian
  case 0: newData = (oldData & 0xFFFFFF00UL) | (data & 0x000000FFUL);        break;
  case 1: newData = (oldData & 0xFFFF00FFUL) | ((data & 0x000000FFUL) << 8);   break;
  case 2: newData = (oldData & 0xFF00FFFFUL) | ((data & 0x000000FFUL) << 16);    break;
  case 3: newData = (oldData & 0x00FFFFFFUL) | ((data & 0x000000FFUL) << 24);    break;
  }

  writeWord(address & ~0x3, newData);
}
