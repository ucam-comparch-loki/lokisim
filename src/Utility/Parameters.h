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
#include "../Types.h"

using std::string;

//============================================================================//
// General definitions
//============================================================================//

typedef unsigned int parameter;

//============================================================================//
// General parameters
//============================================================================//

// Print information about any interesting activity during execution.
extern int      DEBUG;

// Create a trace of instructions executed in a binary format.
extern int      CORE_TRACE;

// Count all events which are significant for energy consumption.
extern int      ENERGY_TRACE;

// Number of bytes in a Loki word.
extern int      BYTES_PER_WORD;

// Number of cycles before terminating execution.
extern unsigned long TIMEOUT;

//============================================================================//
// Global variables
//============================================================================//

// Value to be returned at the end of execution.
extern int      RETURN_CODE;

//============================================================================//
// Architecture size
//============================================================================//

typedef struct {
  size_t size;      // Measured in words (includes register-mapped FIFOs)
} register_file_parameters_t;

typedef struct {
  size_t size;      // Measured in words
} scratchpad_parameters_t;

typedef struct {
  size_t size;      // Measured in entries (number of output channels)
} channel_map_table_parameters_t;

typedef struct {
  size_t size;      // Measured in flits
  bandwidth_t bandwidth; // Measured in flits/cycle. Separate BW for read/write.
} fifo_parameters_t;

typedef struct {
  size_t size;      // Measured in words
  bandwidth_t bandwidth; // Measured in flits/cycle. For writing only.
  size_t numTags;
  size_t maxIPKSize;// Maximum number of instructions in a packet
} cache_parameters_t;

typedef struct {
  size_t numInputChannels; // Includes both instructions and data

  cache_parameters_t              cache;
  register_file_parameters_t      registerFile;
  scratchpad_parameters_t         scratchpad;
  channel_map_table_parameters_t  channelMapTable;
  fifo_parameters_t               ipkFIFO;
  fifo_parameters_t               inputFIFO;
  fifo_parameters_t               outputFIFO;

  size_t numOutputChannels() const;
} core_parameters_t;

typedef struct {
  size_t size;          // Measured in bytes
  size_t cacheLineSize; // Measured in bytes
  uint   latency;       // Total core -> memory -> core latency, in cycles
  bool   hitUnderMiss;  // Is the bank able to serve a new request while waiting
                        // for data for a different request?

  fifo_parameters_t inputFIFO;
  fifo_parameters_t outputFIFO;

  size_t log2CacheLineSize() const;
} memory_bank_parameters_t;

typedef struct {
  size_t size;      // Measured in entries
} directory_parameters_t;

typedef struct {
  fifo_parameters_t fifo;
} router_parameters_t;

typedef struct {
  size_t numCores;
  size_t numMemories;

  core_parameters_t core;
  memory_bank_parameters_t memory;
  directory_parameters_t directory;

  size_t mcastNetInputs() const;
  size_t mcastNetOutputs() const;
  size_t totalComponents() const;
} tile_parameters_t;

// Some duplication between this and memory_bank_parameters_t. Merge?
typedef struct {
  size_t size;          // Measured in bytes
  size_t cacheLineSize; // Measured in bytes
  uint   latency;       // Cycles between receiving a request and sending a response
  uint   bandwidth;     // Measured in words per cycle

  size_t log2CacheLineSize() const;
} main_memory_parameters_t;

typedef struct {
  size2d_t          numComputeTiles;

  tile_parameters_t tile;
  main_memory_parameters_t memory;
  router_parameters_t router;

  size2d_t allTiles() const; // Compute tiles + I/O tiles
  size_t totalCores() const;
  size_t totalMemories() const;
  size_t totalComponents() const;
} chip_parameters_t;

// Number of cores in each tile.
extern parameter  CORES_PER_TILE;

<<<<<<< Upstream, based on origin/master
=======
// Number of accelerators in an accelerator tile.
extern parameter  ACCELERATORS_PER_TILE;

// Number of memory banks in each tile.
extern parameter  MEMS_PER_TILE;

// Number of rows of compute tiles.
extern parameter  COMPUTE_TILE_ROWS;

// Number of columns of compute tiles.
extern parameter  COMPUTE_TILE_COLUMNS;

// Including halo I/O tiles.
#define TOTAL_TILE_ROWS (COMPUTE_TILE_ROWS + 2)
#define TOTAL_TILE_COLUMNS (COMPUTE_TILE_COLUMNS + 2)

//============================================================================//
// Core configuration
//============================================================================//

// Registers accessible through the instruction set.
extern parameter  NUM_ADDRESSABLE_REGISTERS;

// Registers accessible through indirect access (irdr, iwtr).
extern parameter  NUM_PHYSICAL_REGISTERS;

// Size of scratchpad in words.
extern parameter  CORE_SCRATCHPAD_SIZE;

// Size of instruction packet FIFO in words.
extern parameter  IPK_FIFO_SIZE;

// Size of instruction packet cache in words.
extern parameter  IPK_CACHE_SIZE;

// Number of tags in instruction packet cache.
extern parameter  IPK_CACHE_TAGS;

// Number of entries in core's channel map table.
extern parameter  CHANNEL_MAP_SIZE;

// Maximum safe instruction packet size.
extern parameter  MAX_IPK_SIZE;

// Number of entries in the L1 directory, mapping memory addresses to tiles.
extern parameter  DIRECTORY_SIZE;

//============================================================================//
// Memory configuration
//============================================================================//

// Size of memory bank in bytes.
extern parameter  MEMORY_BANK_SIZE;

// Whether a memory bank can serve "hit" requests while waiting for data from
// a miss.
extern parameter  MEMORY_HIT_UNDER_MISS;

// Total core-memory-core latency (assuming a cache hit).
extern parameter  MEMORY_BANK_LATENCY;

// Access latency of main memory once it receives a request.
extern parameter  MAIN_MEMORY_LATENCY;

// Size of main memory in bytes.
extern parameter  MAIN_MEMORY_SIZE;

// Total bandwidth to/from main memory in words per cycle.
extern parameter  MAIN_MEMORY_BANDWIDTH;

>>>>>>> 04c0395 Give accelerators a magic connection to memory, with scope to give something more realistic in future.
// If set to 1, all memory operations complete instantaneously.
extern parameter  MAGIC_MEMORY;


//============================================================================//
// Methods
//============================================================================//

class Parameters {
public:
  // Change a parameter value. Only valid before the SystemC components have
  // been instantiated.
  static void parseParameter(string name, string value,
                             chip_parameters_t& params);

  // Print parameters in a human-readable format to stdout.
  static void printParameters(const chip_parameters_t& params);

  // Print parameters and descriptions to stdout.
  static void printHelp();

  // Print parameters in an XML format.
  static void printParametersXML(std::ostream& os, const chip_parameters_t& params);

  // Populate a struct of parameters with some sensible defaults. These can
  // then be overridden using parseParameter.
  static chip_parameters_t* defaultParameters();

};

#endif /* PARAMETERS_H_ */
