/*
 * CacheLineBuffer.cpp
 *
 *  Created on: 1 Jul 2015
 *      Author: db434
 */

#include "CacheLineBuffer.h"
#include "../../Utility/Parameters.h"
#include <assert.h>

CacheLineBuffer::CacheLineBuffer() {
  mCursor = 0;
  mTag = 0;
}

CacheLineBuffer::~CacheLineBuffer() {
  // Nothing.
}

// Returns whether any of the data in the buffer has been accessed.
bool CacheLineBuffer::startedOperation() const {
  return mCursor > 0;
}

// Returns whether all of the data in the buffer has been accessed.
bool CacheLineBuffer::finishedOperation() const {
  return mCursor >= CACHE_LINE_WORDS;
}

// Move data from cache to cache line buffer.
void CacheLineBuffer::fill(MemoryAddr address, uint32_t data[]) {
  memcpy(mData, data, CACHE_LINE_WORDS*BYTES_PER_WORD);
  setTag(address);
}

void CacheLineBuffer::setTag(MemoryAddr address) {
  mTag = address;
  mCursor = 0;
}

// Get the tag of this cache line.
MemoryAddr CacheLineBuffer::getTag() const {
  return mTag;
}

// Read next word from cache line buffer.
uint32_t CacheLineBuffer::read() {
  uint32_t data = mData[mCursor];

  mCursor++;
  assert(mCursor <= CACHE_LINE_WORDS);

  return data;
}

// Write next word to cache line buffer.
void CacheLineBuffer::write(uint32_t data) {
  assert(mCursor < CACHE_LINE_WORDS);
  mData[mCursor] = data;
  mCursor++;
}

// Move data from cache line buffer to cache.
void CacheLineBuffer::flush(uint32_t data[]) {
  memcpy(data, mData, CACHE_LINE_WORDS*BYTES_PER_WORD);
  mCursor = 0;
}

// The effective address of the next word to be read/written.
MemoryAddr CacheLineBuffer::getCurrentAddress() const {
  return mTag + mCursor*BYTES_PER_WORD;
}

// Returns whether the address is held in this buffer.
bool CacheLineBuffer::inBuffer(MemoryAddr address) {
  mCursor = 0;
  return (address & 0xFFFFFFE0) == mTag;
}
