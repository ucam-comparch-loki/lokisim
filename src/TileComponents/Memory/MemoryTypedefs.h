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
#define CACHE_LINE_BYTES (CACHE_LINE_WORDS * BYTES_PER_WORD)

// A location in the global address space. (Byte granularity.)
typedef uint32_t MemoryAddr;

// In practice, the tag is slightly shorter than the full address.
typedef MemoryAddr MemoryTag;

// The physical location of an item in SRAM. (Byte granularity.)
typedef uint32_t SRAMAddress;


// The bottom bit of the value aliases with the end-of-packet bit.
enum MemoryOperation {
  // End-of-packet flits
  LOAD_W                  = ( 0 << 1) | 1,
  LOAD_LINKED             = ( 1 << 1) | 1,
  LOAD_HW                 = ( 2 << 1) | 1,
  LOAD_B                  = ( 3 << 1) | 1,

  FETCH_LINE              = ( 4 << 1) | 1,
  IPK_READ                = ( 5 << 1) | 1,
  VALIDATE_LINE           = ( 6 << 1) | 1,
  PREFETCH_LINE           = ( 7 << 1) | 1,
  FLUSH_LINE              = ( 8 << 1) | 1,
  INVALIDATE_LINE         = ( 9 << 1) | 1,
  FLUSH_ALL_LINES         = (10 << 1) | 1,
  INVALIDATE_ALL_LINES    = (11 << 1) | 1,

  UPDATE_DIRECTORY_ENTRY  = (12 << 1) | 1,
  UPDATE_DIRECTORY_MASK   = (13 << 1) | 1,

  PAYLOAD_EOP             = (15 << 1) | 1, // End-of-packet payload

  // Start/mid-packet flits
  STORE_W                 = ( 0 << 1) | 0,
  STORE_CONDITIONAL       = ( 1 << 1) | 0,
  STORE_HW                = ( 2 << 1) | 0,
  STORE_B                 = ( 3 << 1) | 0,
  STORE_LINE              = ( 4 << 1) | 0, // Target any bank
  MEMSET_LINE             = ( 5 << 1) | 0, // Target any bank
  PUSH_LINE               = ( 6 << 1) | 0, // Target a particular bank

  LOAD_AND_ADD            = ( 8 << 1) | 0,
  LOAD_AND_OR             = ( 9 << 1) | 0,
  LOAD_AND_AND            = (10 << 1) | 0,
  LOAD_AND_XOR            = (11 << 1) | 0,
  EXCHANGE                = (12 << 1) | 0,

  NONE                    = (14 << 1) | 0,
  PAYLOAD                 = (15 << 1) | 0, // Mid-packet payload
};

inline const std::string& memoryOpName(MemoryOperation op) {
  static const std::string operationNames[] = {
    "STORE_W",      "LOAD_W",                 "STORE_CONDITIONAL", "LOAD_LINKED",
    "STORE_HW",     "LOAD_HW",                "STORE_B",           "LOAD_B",
    "STORE_LINE",   "FETCH_LINE",             "MEMSET_LINE",       "IPK_READ",
    "PUSH_LINE",    "VALIDATE_LINE",          "INVALID14",         "PREFETCH_LINE",
    "LOAD_AND_ADD", "FLUSH_LINE",             "LOAD_AND_OR",       "INVALIDATE_LINE",
    "LOAD_AND_AND", "FLUSH_ALL_LINES",        "LOAD_AND_XOR",      "INVALIDATE_ALL_LINES",
    "EXCHANGE",     "UPDATE_DIRECTORY_ENTRY", "INVALID26",         "UPDATE_DIRECTORY_MASK",
    "NONE",         "INVALID29",              "PAYLOAD",           "PAYLOAD"
  };
  return operationNames[(int)op];
}

struct CacheLookup {
  uint32_t    SRAMLine;      // The physical location of the data in SRAM
  MemoryAddr  FlushAddress;  // Memory address of line to be flushed
  bool        Hit;           // Whether the data is currently in the cache
  bool        FlushRequired; // Whether a cache line flush is required
};

#endif /* MEMORYTYPEDEFS_H_ */
