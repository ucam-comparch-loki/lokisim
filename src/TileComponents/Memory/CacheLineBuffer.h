/*
 * CacheLineBuffer.h
 *
 * A cheap-to-access buffer for dealing with cache line operations. The entire
 * contents can be copied to/from the memory bank when ready.
 *
 *  Created on: 1 Jul 2015
 *      Author: db434
 */

#ifndef SRC_TILECOMPONENTS_MEMORY_CACHELINEBUFFER_H_
#define SRC_TILECOMPONENTS_MEMORY_CACHELINEBUFFER_H_

#include "../../Typedefs.h"
#include "MemoryTypedefs.h"

class CacheLineBuffer {
public:
  CacheLineBuffer();
  virtual ~CacheLineBuffer();

  // Returns whether any of the data in the buffer has been accessed.
  bool startedOperation() const;

  // Returns whether all of the data in the buffer has been accessed.
  bool finishedOperation() const;

  // Move data from cache to cache line buffer and prepare to read.
  void fill(MemoryAddr address, uint32_t data[]);

  // Prepare to write to the buffer.
  void setTag(MemoryAddr address);

  // Get the tag of this cache line.
  MemoryAddr getTag() const;

  // Read next word from cache line buffer.
  uint32_t read();

  // Write next word to cache line buffer.
  void write(uint32_t data);

  // Move data from cache line buffer to cache.
  void flush(uint32_t data[]);

  // The effective address of the next word to be read/written.
  MemoryAddr getCurrentAddress() const;

  // Returns whether the address is held in this buffer.
  bool inBuffer(MemoryAddr address) const;

private:
  // Data buffer.
  uint32_t              mData[CACHE_LINE_WORDS];

  // Index of next word in cache line buffer to process.
  uint                  mCursor;

  // Address of cache line in the buffer.
  MemoryAddr            mTag;
};

#endif /* SRC_TILECOMPONENTS_MEMORY_CACHELINEBUFFER_H_ */
