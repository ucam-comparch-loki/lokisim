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

// An index within a component's own input/output channels.
typedef uint8_t  ChannelIndex;

// An index within a component's own input/output ports.
typedef uint8_t  PortIndex;

// An index within the channel map table.
typedef uint8_t  MapIndex;

// The index of a register within a register file.
typedef uint8_t  RegisterIndex;

// A location in memory.
typedef uint32_t MemoryAddr;

// An offset to jump by in the instruction packet cache.
typedef int16_t  JumpOffset;

typedef uint CacheIndex;
typedef uint TagIndex;

// The topology of the network in each tile.
class LocalNetwork;
typedef LocalNetwork local_net_t;

// The topology of the network between tiles.
class Mesh;
typedef Mesh global_net_t;

using sc_core::sc_in;
using sc_core::sc_out;
using sc_core::sc_signal;

typedef bool IdleType;
typedef sc_signal<IdleType> IdleSignal;
typedef sc_in<IdleType>     IdleInput;
typedef sc_out<IdleType>    IdleOutput;

typedef sc_in<bool>         ClockInput;

#ifndef uint
typedef unsigned int uint;
#endif

#endif /* TYPEDEFS_H_ */
