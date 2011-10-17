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
extern int      BATCH_MODE;    // Print out machine readable tagged information
extern int		CORE_TRACE;	// Enable creation of core trace file
extern int		MEMORY_TRACE;	// Enable creation of memory trace file

extern unsigned long long			TIMEOUT;  // Number of cycles before we assume something's gone wrong.

extern int			BYTES_PER_WORD;

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

extern parameter	CHANNEL_MAP_SIZE;

extern parameter	MAX_IPK_SIZE;

//-------------------------------------------------------------------------------------------------
// Configurable memory system
//-------------------------------------------------------------------------------------------------

extern parameter		MEMORY_CHANNEL_MAP_TABLE_ENTRIES;

extern parameter		MEMORY_BANK_SIZE;

extern parameter		MEMORY_CACHE_RANDOM_REPLACEMENT;

extern parameter		MEMORY_ON_CHIP_SCRATCHPAD_DELAY;
extern parameter		MEMORY_ON_CHIP_SCRATCHPAD_SIZE;
extern parameter		MEMORY_ON_CHIP_SCRATCHPAD_BANKS;

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

#define MEMORY_INPUT_PORTS  1
#define MEMORY_OUTPUT_PORTS 1
#define MEMORY_INPUT_CHANNELS  MEMORY_CHANNEL_MAP_TABLE_ENTRIES
#define MEMORY_OUTPUT_CHANNELS MEMORY_CHANNEL_MAP_TABLE_ENTRIES

extern parameter	IN_CHANNEL_BUFFER_SIZE;
extern parameter  OUT_CHANNEL_BUFFER_SIZE;
extern parameter	ROUTER_BUFFER_SIZE;

//-------------------------------------------------------------------------------------------------
// Combinations of other parameters
//-------------------------------------------------------------------------------------------------

#define				COMPONENTS_PER_TILE  (CORES_PER_TILE + MEMS_PER_TILE)
#define				NUM_TILES            (NUM_TILE_ROWS * NUM_TILE_COLUMNS)

#define				NUM_CORES            (CORES_PER_TILE * NUM_TILES)
#define				NUM_MEMORIES         (MEMS_PER_TILE * NUM_TILES)
#define				NUM_COMPONENTS       (NUM_TILES * COMPONENTS_PER_TILE)

#define				INPUT_PORTS_PER_TILE      (CORES_PER_TILE * CORE_INPUT_PORTS + MEMS_PER_TILE)
#define				OUTPUT_PORTS_PER_TILE     (CORES_PER_TILE * CORE_OUTPUT_PORTS + MEMS_PER_TILE)
#define				TOTAL_INPUT_PORTS         (INPUT_PORTS_PER_TILE * NUM_TILES)
#define				TOTAL_OUTPUT_PORTS        (OUTPUT_PORTS_PER_TILE * NUM_TILES)

#define       INPUT_CHANNELS_PER_TILE   (CORES_PER_TILE * CORE_INPUT_CHANNELS + MEMS_PER_TILE)
#define       OUTPUT_CHANNELS_PER_TILE  (CORES_PER_TILE * CORE_OUTPUT_CHANNELS + MEMS_PER_TILE)
#define       TOTAL_INPUT_CHANNELS      (INPUT_CHANNELS_PER_TILE * NUM_TILES)
#define       TOTAL_OUTPUT_CHANNELS     (OUTPUT_CHANNELS_PER_TILE * NUM_TILES)

//-------------------------------------------------------------------------------------------------
// Run-time parameter management
//-------------------------------------------------------------------------------------------------

class Parameters {
public:
	static void parseParameter(const string &name, const string &value);
	static void printParameters();
};

#endif /* PARAMETERS_H_ */
