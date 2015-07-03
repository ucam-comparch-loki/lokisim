/*
 * MemoryTypedefs.h
 *
 *  Created on: 7 Jan 2015
 *      Author: db434
 */

#ifndef MEMORYTYPEDEFS_H_
#define MEMORYTYPEDEFS_H_

#include <inttypes.h>


#define CACHE_LINE_WORDS 8

// A location in the global address space. (Byte granularity.)
typedef uint32_t MemoryAddr;

// In practice, the tag is slightly shorter than the full address.
typedef MemoryAddr MemoryTag;

// The physical location of an item in SRAM. (Byte granularity.)
typedef uint32_t SRAMAddress;

enum BankMode {
  MODE_INACTIVE,
  MODE_SCRATCHPAD,
  MODE_GP_CACHE
};

// All information used to describe a memory bank's run-time configuration.
struct MemoryConfig {
  BankMode    Mode;          // Scratchpad, cache, etc.
  uint32_t    WayCount;      // Associativity if in cache mode
  uint32_t    LineSize;      // Cache line length (in bytes)
  uint32_t    GroupSize;     // Number of memory banks in group
};

struct CacheLookup {
  uint32_t    SRAMLine;      // The physical location of the data in SRAM
  MemoryAddr  FlushAddress;  // Memory address of line to be flushed
  bool        Hit;           // Whether the data is currently in the cache
  bool        FlushRequired; // Whether a cache line flush is required
};

// Encode line size options efficiently - we're never going to want
// single-word cache lines.
enum LineSizeWords {
  LS_4 = 0,
  LS_8 = 1,
  LS_16 = 2,
  LS_32 = 3,
};

#endif /* MEMORYTYPEDEFS_H_ */
