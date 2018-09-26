/*
 * Parameters.cpp
 *
 * Contains all parameter values.
 *
 * These will eventually be loaded in from an external file, so they can be
 * modified automatically, and without requiring recompilation.
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#include "Parameters.h"
#include "Arguments.h"
#include "Logging.h"
#include "StringManipulation.h"

#include <iostream>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

using std::string;
using std::cout;
using std::cerr;
using std::endl;

//============================================================================//
// General parameters
//============================================================================//

// Print information about any interesting activity during execution.
int DEBUG = 1;

// Print a trace of addresses of instructions executed.
int TRACE = 0;

// Count all events which are significant for energy consumption.
int ENERGY_TRACE = 0;

// Print each instruction executed and its context (register file contents).
// The format should be the same as csim's trace.
int CSIM_TRACE = 0;

// Number of bytes in a Loki word.
int BYTES_PER_WORD = 4;

// Number of cycles before terminating execution.
unsigned long TIMEOUT = ULONG_MAX;

//============================================================================//
// Global variables
//============================================================================//

// Value to be returned at the end of execution.
int RETURN_CODE = EXIT_SUCCESS;

// Number of cores in each tile.
parameter CORES_PER_TILE             = 8;

// If set to 1, all memory operations complete instantaneously.
parameter MAGIC_MEMORY               = 0;


//============================================================================//
// Methods
//============================================================================//

size_t core_parameters_t::numOutputChannels() const {
  return channelMapTable.size;
}

// Used to determine how many bits in a memory address map to the same line.
size_t memory_bank_parameters_t::log2CacheLineSize() const {
  size_t result = 0;
  size_t temp = cacheLineSize >> 1;

  while (temp != 0) {
    result++;
    temp >>= 1;
  }

  assert(1UL << result == cacheLineSize);

  return result;
}

// Used to determine how many bits in a memory address map to the same line.
size_t main_memory_parameters_t::log2CacheLineSize() const {
  size_t result = 0;
  size_t temp = cacheLineSize >> 1;

  while (temp != 0) {
    result++;
    temp >>= 1;
  }

  assert(1UL << result == cacheLineSize);

  return result;
}

size_t tile_parameters_t::mcastNetInputs() const {
  return numCores;
}

size_t tile_parameters_t::mcastNetOutputs() const {
  return numCores;
}

size_t tile_parameters_t::totalComponents() const {
  return numCores + numMemories;
}

size2d_t chip_parameters_t::allTiles() const {
  // There is a border of I/O tiles all around the compute tiles.
  size2d_t tiles = numComputeTiles;
  tiles.height += 2;
  tiles.width += 2;
  return tiles;
}

size_t chip_parameters_t::totalCores() const {
  return numComputeTiles.total() * tile.numCores;
}

size_t chip_parameters_t::totalMemories() const {
  return numComputeTiles.total() * tile.numMemories;
}

size_t chip_parameters_t::totalComponents() const {
  return numComputeTiles.total() * tile.totalComponents();
}

// Change a parameter value. Only valid before the SystemC components have
// been instantiated.
void Parameters::parseParameter(const string &name, const string &value,
                                chip_parameters_t& params) {
  const char* cName = name.c_str();
  int nValue = StringManipulation::strToInt(value);

  // It's probably better to use a dictionary now that there are this many
  // parameters.
  if (strcasecmp(cName, "CORES_PER_TILE") == 0) {params.tile.numCores = nValue; CORES_PER_TILE = nValue;}
  else if (strcasecmp(cName, "MEMS_PER_TILE") == 0) params.tile.numMemories = nValue;
  else if (strcasecmp(cName, "COMPUTE_TILE_ROWS") == 0) params.numComputeTiles.height = nValue;
  else if (strcasecmp(cName, "COMPUTE_TILE_COLUMNS") == 0) params.numComputeTiles.width = nValue;
  else if (strcasecmp(cName, "NUM_ADDRESSABLE_REGISTERS") == 0) params.tile.core.registerFile.size = nValue;
  else if (strcasecmp(cName, "CORE_SCRATCHPAD_SIZE") == 0) params.tile.core.scratchpad.size = nValue;
  else if (strcasecmp(cName, "IPK_FIFO_SIZE") == 0) params.tile.core.ipkFIFO.size = nValue;
  else if (strcasecmp(cName, "IPK_CACHE_SIZE") == 0) params.tile.core.cache.size = nValue;
  else if (strcasecmp(cName, "IPK_CACHE_TAGS") == 0) params.tile.core.cache.numTags = nValue;
  else if (strcasecmp(cName, "CHANNEL_MAP_SIZE") == 0) params.tile.core.channelMapTable.size = nValue;
  else if (strcasecmp(cName, "MAX_IPK_SIZE") == 0) params.tile.core.cache.maxIPKSize = nValue;
  else if (strcasecmp(cName, "DIRECTORY_SIZE") == 0) params.tile.directory.size = nValue;
  else if (strcasecmp(cName, "MEMORY_BANK_LATENCY") == 0) params.tile.memory.latency = nValue;
  else if (strcasecmp(cName, "MEMORY_BANK_SIZE") == 0) params.tile.memory.size = nValue;
  else if (strcasecmp(cName, "MEMORY_HIT_UNDER_MISS") == 0) params.tile.memory.hitUnderMiss = nValue;
  else if (strcasecmp(cName, "MAGIC_MEMORY") == 0) MAGIC_MEMORY = nValue;
  else if (strcasecmp(cName, "MAIN_MEMORY_LATENCY") == 0) params.memory.latency = nValue;
  else if (strcasecmp(cName, "MAIN_MEMORY_SIZE") == 0) params.memory.size = nValue;
  else if (strcasecmp(cName, "MAIN_MEMORY_BANDWIDTH") == 0) params.memory.bandwidth = nValue;
  else if (strcasecmp(cName, "CORE_INPUT_CHANNELS") == 0) params.tile.core.numInputChannels = nValue;
  else if (strcasecmp(cName, "CORE_BUFFER_SIZE") == 0) params.tile.core.inputFIFO.size = nValue;
  else if (strcasecmp(cName, "MEMORY_BUFFER_SIZE") == 0) params.tile.memory.inputFIFO.size = nValue;
  else if (strcasecmp(cName, "ROUTER_BUFFER_SIZE") == 0) params.router.fifo.size = nValue;
  else if (strcasecmp(cName, "DEBUG") == 0) DEBUG = nValue;
  else if (strcasecmp(cName, "TIMEOUT") == 0) TIMEOUT = nValue;
  else {
    LOKI_ERROR << "Encountered unhandled parameter in settings file: " << name << endl;
    throw std::exception();
  }
}

// Print parameters in a human-readable format.
void Parameters::printParameters(const chip_parameters_t& params) {
  cout << "Parameter CORES_PER_TILE is " << params.tile.numCores << endl;
  cout << "Parameter MEMS_PER_TILE is " << params.tile.numMemories << endl;
  cout << "Parameter COMPUTE_TILE_ROWS is " << params.numComputeTiles.height << endl;
  cout << "Parameter COMPUTE_TILE_COLUMNS is " << params.numComputeTiles.width << endl;
  cout << "Parameter NUM_ADDRESSABLE_REGISTERS is " << params.tile.core.registerFile.size << endl;
  cout << "Parameter CORE_SCRATCHPAD_SIZE is " << params.tile.core.scratchpad.size << endl;
  cout << "Parameter IPK_FIFO_SIZE is " << params.tile.core.ipkFIFO.size << endl;
  cout << "Parameter IPK_CACHE_SIZE is " << params.tile.core.cache.size << endl;
  cout << "Parameter IPK_CACHE_TAGS is " << params.tile.core.cache.numTags << endl;
  cout << "Parameter CHANNEL_MAP_SIZE is " << params.tile.core.channelMapTable.size << endl;
  cout << "Parameter MAX_IPK_SIZE is " << params.tile.core.cache.maxIPKSize << endl;
  cout << "Parameter DIRECTORY_SIZE is " << params.tile.directory.size << endl;
  cout << "Parameter MEMORY_BANK_SIZE is " << params.tile.memory.size << endl;
  cout << "Parameter MEMORY_BANK_LATENCY is " << params.tile.memory.latency << endl;
  cout << "Parameter MEMORY_HIT_UNDER_MISS is " << params.tile.memory.hitUnderMiss << endl;
  cout << "Parameter MAIN_MEMORY_LATENCY is " << params.memory.latency << endl;
  cout << "Parameter MAIN_MEMORY_SIZE is " << params.memory.size << endl;
  cout << "Parameter MAIN_MEMORY_BANDWIDTH is " << params.memory.bandwidth << endl;
  cout << "Parameter MAGIC_MEMORY is " << MAGIC_MEMORY << endl;
  cout << "Parameter CORE_INPUT_CHANNELS is " << params.tile.core.numInputChannels << endl;
  cout << "Parameter CORE_BUFFER_SIZE is " << params.tile.core.inputFIFO.size << endl;
  cout << "Parameter MEMORY_BUFFER_SIZE is " << params.tile.memory.inputFIFO.size << endl;
  cout << "Parameter ROUTER_BUFFER_SIZE is " << params.router.fifo.size << endl;
}

#define XML_LINE(name, value) "\t<" << name << ">" << value << "</" << name << ">\n"

// Print parameters in an XML format.
void Parameters::printParametersXML(std::ostream& os, const chip_parameters_t& params) {
  os << "<parameters>\n"
     << XML_LINE("cores_per_tile", params.tile.numCores)
     << XML_LINE("memories_per_tile", params.tile.numMemories)
     << XML_LINE("tile_rows", params.numComputeTiles.height)
     << XML_LINE("tile_columns", params.numComputeTiles.width)
     << XML_LINE("addressable_regs", params.tile.core.registerFile.size)
     << XML_LINE("physical_regs", params.tile.core.registerFile.size)
     << XML_LINE("ipk_cache_size", params.tile.core.cache.size)
     << XML_LINE("ipk_cache_tags", params.tile.core.cache.numTags)
     << XML_LINE("channel_map_size", params.tile.core.channelMapTable.size)
     << XML_LINE("memory_bank_size", params.tile.memory.size)
     << XML_LINE("main_memory_bandwidth", params.memory.bandwidth)
     << "</parameters>\n";
}

chip_parameters_t* Parameters::defaultParameters() {
  chip_parameters_t* p = new chip_parameters_t();

  p->numComputeTiles.width = 1;
  p->numComputeTiles.height = 1;

  p->tile.numCores = 8;
  p->tile.numMemories = 8;

  p->tile.core.numInputChannels = 8;
  p->tile.core.cache.maxIPKSize = 8;  // Cache line size?
  p->tile.core.cache.numTags = 0;  // Set in params.txt
  p->tile.core.cache.size = 0;     // Set in params.txt
  p->tile.core.channelMapTable.size = 16 - 1;  // 1 channel reserved
  p->tile.core.registerFile.size = 32;
  p->tile.core.scratchpad.size = 256;
  p->tile.core.inputFIFO.size = 4;
  p->tile.core.ipkFIFO.size = 24;
  p->tile.core.outputFIFO.size = 4;

  p->tile.memory.size = 8 * 1024;
  p->tile.memory.cacheLineSize = 8 * BYTES_PER_WORD;
  p->tile.memory.latency = 3;
  p->tile.memory.hitUnderMiss = true;
  p->tile.memory.inputFIFO.size = 4;
  p->tile.memory.outputFIFO.size = 4;

  p->tile.directory.size = 16;

  p->memory.size = 256 * 1024 * 1024;
  p->memory.cacheLineSize = p->tile.memory.cacheLineSize;
  p->memory.bandwidth = 1;
  p->memory.latency = 20;

  p->router.fifo.size = 4;

  return p;
}
