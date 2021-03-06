/*
 * Typedefs.h
 *
 * A set of useful typedefs for use throughout the system.
 *
 * These help with consistency, and also make the intended use of a value
 * more clear. (e.g. A RegisterIndex value is obviously used to address
 * registers, but an int may be less clear.)
 *
 *  Created on: 26 Oct 2010
 *      Author: db434
 */

#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

#include <inttypes.h>
#include <systemc>

typedef uint64_t count_t;
typedef count_t  cycle_count_t;
typedef uint     bandwidth_t;

typedef struct {
  size_t width;
  size_t height;

  size_t total() const {return width * height;}
} size2d_t;

// Identifier used for a tile. Compute tiles start at 1,1.
// (column << 3) | row
typedef uint32_t TileIndex;

// Positions of components within a tile.
typedef uint32_t ComponentIndex;
typedef uint32_t CoreIndex;
typedef uint32_t MemoryIndex;

// An index within a component's own input/output channels.
typedef uint32_t ChannelIndex;

// An index within a component's own input/output ports.
typedef uint32_t PortIndex;

// An index within the channel map table.
typedef uint32_t MapIndex;

// The index of a register within a register file.
typedef uint32_t RegisterIndex;

// An offset (in words) to jump by in the instruction packet cache.
typedef int32_t  JumpOffset;

typedef uint CacheIndex;
typedef uint TagIndex;

using sc_core::sc_in;
using sc_core::sc_out;
using sc_core::sc_signal;

typedef sc_in<bool>         ClockInput;

#ifndef uint
typedef unsigned int uint;
#endif

#endif /* TYPEDEFS_H_ */
