/*
 * AbstractMemoryHandler.cpp
 *
 *  Created on: 8 Jan 2015
 *      Author: db434
 */

#include "AbstractMemoryHandler.h"
#include "../../Utility/Parameters.h"

#include <cassert>

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

AbstractMemoryHandler::AbstractMemoryHandler(uint bankNumber) :
    mBankNumber(bankNumber) {

  //-- Configuration parameters -----------------------------------------------------------------

  mSetCount = -1;
  mWayCount = -1;
  mLineSize = -1;

  //-- State ------------------------------------------------------------------------------------

  mData = new uint32_t[MEMORY_BANK_SIZE / 4];

  mLineBits = -1;
  mLineMask = 0;

  mGroupIndex = -1;
  mGroupBits = -1;
  mGroupMask = 0;

}

AbstractMemoryHandler::~AbstractMemoryHandler() {
  delete[] mData;
}

