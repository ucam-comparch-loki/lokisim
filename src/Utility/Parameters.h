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

// Print information about any interesting activity during execution.
extern int			DEBUG;

// Create a trace of instructions executed in a binary format.
extern int      CORE_TRACE;

// Count all events which are significant for energy consumption.
extern int      ENERGY_TRACE;

// Number of cycles before we assume something's gone wrong.
extern unsigned long long			TIMEOUT;

// Number of bytes in a Loki word.
extern int			BYTES_PER_WORD;

//-------------------------------------------------------------------------------------------------
// Global variables (is there a better place to put these?)
//-------------------------------------------------------------------------------------------------

// Value to be returned at the end of execution.
extern int      RETURN_CODE;

//-------------------------------------------------------------------------------------------------
// Architecture size
//-------------------------------------------------------------------------------------------------

// Number of cores in each tile.
extern parameter	CORES_PER_TILE;

// Number of memory banks in each tile.
extern parameter	MEMS_PER_TILE;

// Number of rows of compute tiles.
extern parameter	COMPUTE_TILE_ROWS;

// Number of columns of compute tiles.
extern parameter	COMPUTE_TILE_COLUMNS;

// Including halo I/O tiles.
#define TOTAL_TILE_ROWS (COMPUTE_TILE_ROWS + 2)
#define TOTAL_TILE_COLUMNS (COMPUTE_TILE_COLUMNS + 2)

//-------------------------------------------------------------------------------------------------
// Memory
//-------------------------------------------------------------------------------------------------

// Registers accessible through the instruction set.
extern parameter	NUM_ADDRESSABLE_REGISTERS;

// Registers accessible through indirect access (irdr, iwtr).
extern parameter	NUM_PHYSICAL_REGISTERS;

// Size of scratchpad in words.
extern parameter  CORE_SCRATCHPAD_SIZE;

// Size of instruction packet FIFO in words.
extern parameter	IPK_FIFO_SIZE;

// Size of instruction packet cache in words.
extern parameter	IPK_CACHE_SIZE;

// Number of tags in instruction packet cache.
extern parameter  IPK_CACHE_TAGS;

// Number of entries in core's channel map table.
extern parameter	CHANNEL_MAP_SIZE;

// Maximum safe instruction packet size.
extern parameter	MAX_IPK_SIZE;

// Number of entries in the L1 directory, mapping memory addresses to tiles.
extern parameter  DIRECTORY_SIZE;

// Total core-memory-core latency (assuming a cache hit).
extern parameter  L1_LATENCY;

// If set to 1, all memory operations complete instantaneously.
extern parameter  MAGIC_MEMORY;

//-------------------------------------------------------------------------------------------------
// Configurable memory system
//-------------------------------------------------------------------------------------------------

// Size of memory bank in bytes.
extern parameter		MEMORY_BANK_SIZE;

// Memory bank replacement policy. 0 = ideal LRU, 1 = random.
extern parameter		MEMORY_CACHE_RANDOM_REPLACEMENT;

// Access latency of "main memory" once it receives a request.
extern parameter		MEMORY_ON_CHIP_SCRATCHPAD_DELAY;

// Size of "main memory" in bytes.
extern parameter		MEMORY_ON_CHIP_SCRATCHPAD_SIZE;

// Maximum number of concurrent accesses to "main memory".
extern parameter		MEMORY_ON_CHIP_SCRATCHPAD_BANKS;

// Whether a memory bank can serve "hit" requests while waiting for data from
// a miss.
extern parameter    MEMORY_HIT_UNDER_MISS;

//-------------------------------------------------------------------------------------------------
// Network
//
// Note: a port is a physical connection to the network, whereas a channel is
// an address accessible through the network. There may be many channels
// accessible through each port.
//-------------------------------------------------------------------------------------------------

// Number of connections from the various networks to the core.
extern parameter  CORE_INPUT_PORTS;

// Number of connections from the core to the various networks.
extern parameter  CORE_OUTPUT_PORTS;

// Number of register-mapped input buffers in cores.
extern parameter	NUM_RECEIVE_CHANNELS;

// Number of core input channels, including instruction inputs.
#define           CORE_INPUT_CHANNELS       (2 + NUM_RECEIVE_CHANNELS)

// Number of core output channels.
#define           CORE_OUTPUT_CHANNELS      (CHANNEL_MAP_SIZE)

// Number of connections from networks to each memory bank.
#define           MEMORY_INPUT_PORTS        1

// Number of connections from each memory bank to networks.
#define           MEMORY_OUTPUT_PORTS       1

// Number of addressable channels accessible on each memory bank.
#define           MEMORY_INPUT_CHANNELS     MEMORY_CHANNEL_MAP_TABLE_ENTRIES

// Number of channels from which a memory bank can send data.
#define           MEMORY_OUTPUT_CHANNELS    MEMORY_CHANNEL_MAP_TABLE_ENTRIES

// Number of entries of core input buffers.
extern parameter	IN_CHANNEL_BUFFER_SIZE;

// Number of entries of core output buffers.
extern parameter  OUT_CHANNEL_BUFFER_SIZE;

// Number of entries of router buffers.
extern parameter	ROUTER_BUFFER_SIZE;

//-------------------------------------------------------------------------------------------------
// Combinations of other parameters
//-------------------------------------------------------------------------------------------------

// Total number of addressable components (cores and memory banks) on each tile.
#define				COMPONENTS_PER_TILE       (CORES_PER_TILE + MEMS_PER_TILE)

// Total number of tiles on the chip.
#define       NUM_TILES                 (TOTAL_TILE_ROWS * TOTAL_TILE_COLUMNS)
#define				NUM_COMPUTE_TILES         (COMPUTE_TILE_ROWS * COMPUTE_TILE_COLUMNS)

// Total number of cores on the chip.
#define				NUM_CORES                 (CORES_PER_TILE * NUM_COMPUTE_TILES)

// Total number of L1 memory banks on the chip.
#define				NUM_MEMORIES              (MEMS_PER_TILE * NUM_COMPUTE_TILES)

// Total number of addressable components (cores and memory banks) on the chip.
#define				NUM_COMPONENTS            (NUM_COMPUTE_TILES * COMPONENTS_PER_TILE)

// Useful numbers for generating networks.
#define				INPUT_PORTS_PER_TILE      (CORES_PER_TILE * CORE_INPUT_PORTS + MEMS_PER_TILE)
#define				OUTPUT_PORTS_PER_TILE     (CORES_PER_TILE * CORE_OUTPUT_PORTS + MEMS_PER_TILE)
#define				TOTAL_INPUT_PORTS         (INPUT_PORTS_PER_TILE * NUM_COMPUTE_TILES)
#define				TOTAL_OUTPUT_PORTS        (OUTPUT_PORTS_PER_TILE * NUM_COMPUTE_TILES)

// Useful numbers for generating networks.
#define       INPUT_CHANNELS_PER_TILE   (CORES_PER_TILE * CORE_INPUT_CHANNELS + MEMS_PER_TILE)
#define       OUTPUT_CHANNELS_PER_TILE  (CORES_PER_TILE * CORE_OUTPUT_CHANNELS + MEMS_PER_TILE)
#define       TOTAL_INPUT_CHANNELS      (INPUT_CHANNELS_PER_TILE * NUM_COMPUTE_TILES)
#define       TOTAL_OUTPUT_CHANNELS     (OUTPUT_CHANNELS_PER_TILE * NUM_COMPUTE_TILES)

//-------------------------------------------------------------------------------------------------
// Run-time parameter management
//-------------------------------------------------------------------------------------------------

class Parameters {
public:
  // Change a parameter value. Only valid before the SystemC components have
  // been instantiated.
	static void parseParameter(const string &name, const string &value);

	// Print parameters to stdout.
  static void printParameters();

	// Print parameters in an XML format.
	static void printParametersXML(std::ostream& os);
};

#endif /* PARAMETERS_H_ */
