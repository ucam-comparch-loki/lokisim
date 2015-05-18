/*
 * MemoryTypedefs.h
 *
 *  Created on: 7 Jan 2015
 *      Author: db434
 */

#ifndef MEMORYTYPEDEFS_H_
#define MEMORYTYPEDEFS_H_


#define CACHE_LINE_WORDS 8

enum BankMode {
  MODE_INACTIVE,
  MODE_SCRATCHPAD,
  MODE_GP_CACHE
};

// All information used to describe a memory bank's run-time configuration.
struct MemoryConfig_ {
  BankMode Mode;          // Scratchpad, cache, etc.
  uint     WayCount;      // Associativity if in cache mode
  uint     LineSize;      // Cache line length (in bytes)
  uint     GroupBaseBank; // Index of first memory bank in this group
  uint     GroupIndex;    // Index of this memory bank within group
  uint     GroupSize;     // Number of memory banks in group
};
typedef struct MemoryConfig_ MemoryConfig;

// Encode line size options efficiently - we're never going to want
// single-word cache lines.
enum LineSizeWords {
  LS_4 = 0,
  LS_8 = 1,
  LS_16 = 2,
  LS_32 = 3,
};

#endif /* MEMORYTYPEDEFS_H_ */
