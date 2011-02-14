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

extern int			TIMEOUT;

extern int			BYTES_PER_WORD;
extern int      BYTES_PER_INSTRUCTION;

//-------------------------------------------------------------------------------------------------
// Global variables (is there a better place to put these?)
//-------------------------------------------------------------------------------------------------

extern int      RETURN_CODE;
extern int      LOKI_STDIN;
extern int      LOKI_STDOUT;
extern int      LOKI_STDERR;

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
//-------------------------------------------------------------------------------------------------

extern parameter	NUM_RECEIVE_CHANNELS;
#define           NUM_SEND_CHANNELS			(CHANNEL_MAP_SIZE)
extern parameter	NUM_MEMORY_INPUTS;
#define           NUM_MEMORY_OUTPUTS			(NUM_MEMORY_INPUTS)
extern parameter	CHANNEL_END_BUFFER_SIZE;	// Different send/receive sizes?
extern parameter	ROUTER_BUFFER_SIZE;
extern parameter	NETWORK_BUFFER_SIZE;

extern parameter	WORMHOLE_ROUTING;

//-------------------------------------------------------------------------------------------------
// Combinations of other parameters
//-------------------------------------------------------------------------------------------------

#define				NUM_CORE_INPUTS				(2 + NUM_RECEIVE_CHANNELS)
#define				NUM_CORE_OUTPUTS			(NUM_SEND_CHANNELS)
#define				COMPONENTS_PER_TILE			(CORES_PER_TILE + ((ENABLE_SHARED_L1_CACHE_SUBSYSTEM == 0) ? MEMS_PER_TILE : 1))
#define				NUM_TILES					(NUM_TILE_ROWS * NUM_TILE_COLUMNS)

#define				NUM_CORES					(CORES_PER_TILE * NUM_TILES)
#define				NUM_MEMORIES				((ENABLE_SHARED_L1_CACHE_SUBSYSTEM == 0) ? (MEMS_PER_TILE * NUM_TILES) : (NUM_TILES))
#define				NUM_COMPONENTS				(NUM_TILES * COMPONENTS_PER_TILE)

#define				INPUTS_PER_TILE				(CORES_PER_TILE * NUM_CORE_INPUTS + ((ENABLE_SHARED_L1_CACHE_SUBSYSTEM == 0) ? MEMS_PER_TILE : 1) * NUM_MEMORY_INPUTS)
#define				OUTPUTS_PER_TILE			(CORES_PER_TILE * NUM_CORE_OUTPUTS + ((ENABLE_SHARED_L1_CACHE_SUBSYSTEM == 0) ? MEMS_PER_TILE : 1) * NUM_MEMORY_OUTPUTS)
#define				TOTAL_INPUTS				(INPUTS_PER_TILE * NUM_TILES)
#define				TOTAL_OUTPUTS				(OUTPUTS_PER_TILE * NUM_TILES)

//-------------------------------------------------------------------------------------------------
// Run-time parameter management
//-------------------------------------------------------------------------------------------------

class Parameters {
public:
	static void parseParameter(const string &name, const string &value);
};

#endif /* PARAMETERS_H_ */
