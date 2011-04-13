/*
 * Parameters.h
 *
 * Contains a list of all parameters in the program. The values are held in
 * Parameters.cpp, to prevent mass re-compilation when a value is changed.
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#ifndef PARAMETERS_H_
#define PARAMETERS_H_

#include <string>

using std::string;

//-------------------------------------------------------------------------------------------------
// General definitions
//-------------------------------------------------------------------------------------------------

typedef unsigned int parameter;

//-------------------------------------------------------------------------------------------------
// General parameters
//-------------------------------------------------------------------------------------------------

extern int			DEBUG;    // Print out lots of information during execution
extern int      TRACE;    // Print out only the addresses of executed instructions

extern int			TIMEOUT;  // Number of cycles before we assume something's gone wrong.

extern int			BYTES_PER_WORD;
extern int      BYTES_PER_INSTRUCTION;

//-------------------------------------------------------------------------------------------------
// Global variables (is there a better place to put these?)
//-------------------------------------------------------------------------------------------------

extern int      RETURN_CODE;

//-------------------------------------------------------------------------------------------------
// Architecture size
//-------------------------------------------------------------------------------------------------

extern parameter	CORES_PER_TILE;
extern parameter	MEMS_PER_TILE;

extern parameter	NUM_TILE_ROWS;
extern parameter	NUM_TILE_COLUMNS;

//-------------------------------------------------------------------------------------------------
// Memory
//-------------------------------------------------------------------------------------------------

extern parameter	NUM_ADDRESSABLE_REGISTERS;
extern parameter	NUM_PHYSICAL_REGISTERS;
extern parameter	IPK_FIFO_SIZE;
extern parameter	IPK_CACHE_SIZE;
extern parameter	MEMORY_SIZE;
extern parameter	CONCURRENT_MEM_OPS;

extern parameter	CHANNEL_MAP_SIZE;

extern parameter	MAX_IPK_SIZE;

//-------------------------------------------------------------------------------------------------
// Shared L1 cache subsystem
//-------------------------------------------------------------------------------------------------

extern parameter	ENABLE_SHARED_L1_CACHE_SUBSYSTEM;

extern parameter	SHARED_L1_CACHE_CHANNELS;
extern parameter	SHARED_L1_CACHE_INTERFACE_QUEUE_DEPTH;

extern parameter	SHARED_L1_CACHE_BANKS;
extern parameter	SHARED_L1_CACHE_SETS_PER_BANK;
extern parameter	SHARED_L1_CACHE_ASSOCIATIVITY;
extern parameter	SHARED_L1_CACHE_LINE_SIZE;

extern parameter	SHARED_L1_CACHE_SEQUENTIAL_SEARCH;
extern parameter	SHARED_L1_CACHE_RANDOM_REPLACEMENT;

extern parameter	SHARED_L1_CACHE_MEMORY_QUEUE_DEPTH;
extern parameter	SHARED_L1_CACHE_MEMORY_DELAY_CYCLES;

//-------------------------------------------------------------------------------------------------
// Network
//
// Note: a port is a physical connection to the network, whereas a channel is
// an address accessible through the network. There may be many channels
// accessible through each port.
//-------------------------------------------------------------------------------------------------

extern parameter  CORE_INPUT_PORTS;
extern parameter  CORE_OUTPUT_PORTS;
extern parameter	NUM_RECEIVE_CHANNELS;     // Register-mapped inputs only
#define           CORE_INPUT_CHANNELS       (2 + NUM_RECEIVE_CHANNELS)
#define           CORE_OUTPUT_CHANNELS      (CHANNEL_MAP_SIZE)

extern parameter	MEMORY_INPUT_PORTS;
extern parameter  MEMORY_OUTPUT_PORTS;
extern parameter  MEMORY_INPUT_CHANNELS;
extern parameter  MEMORY_OUTPUT_CHANNELS;

extern parameter	CHANNEL_END_BUFFER_SIZE;	// Different send/receive sizes?
extern parameter	ROUTER_BUFFER_SIZE;
extern parameter	NETWORK_BUFFER_SIZE;

extern parameter	WORMHOLE_ROUTING;

//-------------------------------------------------------------------------------------------------
// Combinations of other parameters
//-------------------------------------------------------------------------------------------------

#define				COMPONENTS_PER_TILE  (CORES_PER_TILE + ((ENABLE_SHARED_L1_CACHE_SUBSYSTEM == 0) ? MEMS_PER_TILE : 1))
#define				NUM_TILES            (NUM_TILE_ROWS * NUM_TILE_COLUMNS)

#define				NUM_CORES            (CORES_PER_TILE * NUM_TILES)
#define				NUM_MEMORIES         ((ENABLE_SHARED_L1_CACHE_SUBSYSTEM == 0) ? (MEMS_PER_TILE * NUM_TILES) : (NUM_TILES))
#define				NUM_COMPONENTS       (NUM_TILES * COMPONENTS_PER_TILE)

#define				INPUT_PORTS_PER_TILE      (CORES_PER_TILE * CORE_INPUT_PORTS + ((ENABLE_SHARED_L1_CACHE_SUBSYSTEM == 0) ? MEMS_PER_TILE : 1) * MEMORY_INPUT_PORTS)
#define				OUTPUT_PORTS_PER_TILE     (CORES_PER_TILE * CORE_OUTPUT_PORTS + ((ENABLE_SHARED_L1_CACHE_SUBSYSTEM == 0) ? MEMS_PER_TILE : 1) * MEMORY_OUTPUT_PORTS)
#define				TOTAL_INPUT_PORTS         (INPUT_PORTS_PER_TILE * NUM_TILES)
#define				TOTAL_OUTPUT_PORTS        (OUTPUT_PORTS_PER_TILE * NUM_TILES)

#define       INPUT_CHANNELS_PER_TILE   (CORES_PER_TILE * CORE_INPUT_CHANNELS + ((ENABLE_SHARED_L1_CACHE_SUBSYSTEM == 0) ? MEMS_PER_TILE : 1) * MEMORY_INPUT_CHANNELS)
#define       OUTPUT_CHANNELS_PER_TILE  (CORES_PER_TILE * CORE_OUTPUT_CHANNELS + ((ENABLE_SHARED_L1_CACHE_SUBSYSTEM == 0) ? MEMS_PER_TILE : 1) * MEMORY_OUTPUT_CHANNELS)
#define       TOTAL_INPUT_CHANNELS      (INPUT_CHANNELS_PER_TILE * NUM_TILES)
#define       TOTAL_OUTPUT_CHANNELS     (OUTPUT_CHANNELS_PER_TILE * NUM_TILES)

//-------------------------------------------------------------------------------------------------
// Run-time parameter management
//-------------------------------------------------------------------------------------------------

class Parameters {
public:
	static void parseParameter(const string &name, const string &value);
};

#endif /* PARAMETERS_H_ */
