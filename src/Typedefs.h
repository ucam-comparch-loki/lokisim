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

// A unique identifier for each core and memory on the chip.
typedef uint32_t ComponentID;

// An index within a component's own input/output channels.
typedef uint8_t  ChannelIndex;

// An index within the channel map table.
typedef uint8_t  MapIndex;

// The global address of a component's input/output port.
typedef uint32_t ChannelID;

// The index of a register within a register file.
typedef uint8_t  RegisterIndex;

// A location in memory.
typedef uint32_t MemoryAddr;



#endif /* TYPEDEFS_H_ */
