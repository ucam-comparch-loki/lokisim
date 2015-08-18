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
#include <stdlib.h>
#include <string.h>

using std::string;
using std::cout;
using std::cerr;
using std::endl;

//-------------------------------------------------------------------------------------------------
// General parameters
//-------------------------------------------------------------------------------------------------

// Print information about any interesting activity during execution.
int DEBUG = 1;

// Print a trace of addresses of instructions executed.
int TRACE = 0;

// Print out information in a machine-readable form.
int BATCH_MODE = 0;

// Create a trace of instructions executed in a binary format.
int CORE_TRACE = 0;

// Create a trace of all memory accesses (L1 and lower).
int MEMORY_TRACE = 0;

// Count all events which are significant for energy consumption.
int ENERGY_TRACE = 0;

// Take snapshots of register file contents, allowing simulation to continue on
// a different simulator.
int SOFTWARE_TRACE = 0;

// Log all kinds of execution information in the LBT format. (???)
int LBT_TRACE = 0;

// Print each instruction executed and its context (register file contents).
// The format should be the same as csim's trace.
int CSIM_TRACE = 0;

// Number of cycles before we assume something's gone wrong.
unsigned long long TIMEOUT = 1000000000000ULL;

// Number of bytes in a Loki word.
int BYTES_PER_WORD = 4;

//-------------------------------------------------------------------------------------------------
// Global variables (is there a better place to put these?)
//-------------------------------------------------------------------------------------------------

// Value to be returned at the end of execution.
int RETURN_CODE = EXIT_SUCCESS;

//-------------------------------------------------------------------------------------------------
// Architecture size
//-------------------------------------------------------------------------------------------------

// Number of cores in each tile.
parameter CORES_PER_TILE             = 8;

// Number of memory banks in each tile.
parameter MEMS_PER_TILE              = 8;

// Number of rows of compute tiles.
parameter COMPUTE_TILE_ROWS          = 1;

// Number of columns of compute tiles.
parameter COMPUTE_TILE_COLUMNS       = 1;

//-------------------------------------------------------------------------------------------------
// Memory
//-------------------------------------------------------------------------------------------------

// Registers accessible through the instruction set.
parameter NUM_ADDRESSABLE_REGISTERS  = 32;

// Registers accessible through indirect access (irdr, iwtr).
parameter NUM_PHYSICAL_REGISTERS     = 32;

// Size of scratchpad in words.
parameter CORE_SCRATCHPAD_SIZE       = 256;

// Size of instruction packet FIFO in words.
parameter IPK_FIFO_SIZE              = 24;

// Size of instruction packet cache in words. Set in params.txt.
parameter IPK_CACHE_SIZE             = 0;

// Number of tags in instruction packet cache. Set in params.txt.
parameter IPK_CACHE_TAGS             = 0;

// Number of entries in core's channel map table.
parameter CHANNEL_MAP_SIZE           = 16 - 1; // Final entry is reserved as NULL

// Maximum safe instruction packet size.
parameter MAX_IPK_SIZE               = 8; // Fetch one cache line at a time.

// Number of entries in the L1 directory, mapping memory addresses to tiles.
parameter DIRECTORY_SIZE             = 16;

// Total core-memory-core latency (assuming a cache hit).
parameter L1_LATENCY                 = 3;

//-------------------------------------------------------------------------------------------------
// Configurable memory system
//-------------------------------------------------------------------------------------------------

// Size of memory bank in bytes.
parameter MEMORY_BANK_SIZE                  = 8192;		// 8 KB per bank

// Memory bank replacement policy. 0 = ideal LRU, 1 = random.
parameter MEMORY_CACHE_RANDOM_REPLACEMENT		= 1;

// Access latency of "main memory" once it receives a request.
parameter MEMORY_ON_CHIP_SCRATCHPAD_DELAY		= 20;

// Size of "main memory" in bytes.
parameter MEMORY_ON_CHIP_SCRATCHPAD_SIZE		= 32 * 1024 * 1024;

// Maximum number of concurrent accesses to "main memory".
parameter MEMORY_ON_CHIP_SCRATCHPAD_BANKS		= 4;

// Whether a memory bank can serve "hit" requests while waiting for data from
// a miss.
parameter MEMORY_HIT_UNDER_MISS             = 1;

//-------------------------------------------------------------------------------------------------
// Network
//
// Note: a port is a physical connection to the network, whereas a channel is
// an address accessible through the network. There may be many channels
// accessible through each port.
//-------------------------------------------------------------------------------------------------

// Number of connections from the various networks to the core.
parameter CORE_INPUT_PORTS         = 4;  // 2 from cores, 2 from memories

// Number of connections from the core to the various networks.
parameter CORE_OUTPUT_PORTS        = 1;

// Number of register-mapped input buffers in cores.
parameter NUM_RECEIVE_CHANNELS     = 6;

// Number of entries of core input buffers.
parameter IN_CHANNEL_BUFFER_SIZE   = 4;

// Number of entries of core output buffers.
parameter OUT_CHANNEL_BUFFER_SIZE  = 4;

// Number of entries of router buffers.
parameter ROUTER_BUFFER_SIZE       = 4;

//-------------------------------------------------------------------------------------------------
// Run-time parameter management
//-------------------------------------------------------------------------------------------------

// If "name" matches the name of the parameter, set the parameter to the value.
#define SET_IF_MATCH(name, value, param) if(strcasecmp(name, #param)==0)\
  param = value

// Change a parameter value. Only valid before the SystemC components have
// been instantiated.
void Parameters::parseParameter(const string &name, const string &value) {
	const char* cName = name.c_str();
	int nValue = StringManipulation::strToInt(value);

  SET_IF_MATCH(cName, nValue, TIMEOUT);
  else SET_IF_MATCH(cName, nValue, CORES_PER_TILE);
	else SET_IF_MATCH(cName, nValue, MEMS_PER_TILE);
	else SET_IF_MATCH(cName, nValue, COMPUTE_TILE_ROWS);
	else SET_IF_MATCH(cName, nValue, COMPUTE_TILE_COLUMNS);
	else SET_IF_MATCH(cName, nValue, NUM_ADDRESSABLE_REGISTERS);
	else SET_IF_MATCH(cName, nValue, NUM_PHYSICAL_REGISTERS);
  else SET_IF_MATCH(cName, nValue, CORE_SCRATCHPAD_SIZE);
	else SET_IF_MATCH(cName, nValue, IPK_FIFO_SIZE);
  else SET_IF_MATCH(cName, nValue, IPK_CACHE_SIZE);
  else SET_IF_MATCH(cName, nValue, IPK_CACHE_TAGS);
	else SET_IF_MATCH(cName, nValue, CHANNEL_MAP_SIZE);
  else SET_IF_MATCH(cName, nValue, MAX_IPK_SIZE);
  else SET_IF_MATCH(cName, nValue, DIRECTORY_SIZE);
  else SET_IF_MATCH(cName, nValue, L1_LATENCY);
	else SET_IF_MATCH(cName, nValue, MEMORY_BANK_SIZE);
	else SET_IF_MATCH(cName, nValue, MEMORY_CACHE_RANDOM_REPLACEMENT);
	else SET_IF_MATCH(cName, nValue, MEMORY_ON_CHIP_SCRATCHPAD_DELAY);
	else SET_IF_MATCH(cName, nValue, MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
  else SET_IF_MATCH(cName, nValue, MEMORY_ON_CHIP_SCRATCHPAD_BANKS);
  else SET_IF_MATCH(cName, nValue, MEMORY_HIT_UNDER_MISS);
	else SET_IF_MATCH(cName, nValue, NUM_RECEIVE_CHANNELS);
	else SET_IF_MATCH(cName, nValue, IN_CHANNEL_BUFFER_SIZE);
  else SET_IF_MATCH(cName, nValue, OUT_CHANNEL_BUFFER_SIZE);
  else SET_IF_MATCH(cName, nValue, ROUTER_BUFFER_SIZE);
  else SET_IF_MATCH(cName, nValue, DEBUG);
	else {
		LOKI_ERROR << "Encountered unhandled parameter in settings file: " << name << endl;
		throw std::exception();
	}
}

void Parameters::printParameters() {
  cout << "Parameter CORES_PER_TILE is " << CORES_PER_TILE << endl;
  cout << "Parameter MEMS_PER_TILE is " << MEMS_PER_TILE << endl;
  cout << "Parameter COMPUTE_TILE_ROWS is " << COMPUTE_TILE_ROWS << endl;
  cout << "Parameter COMPUTE_TILE_COLUMNS is " << COMPUTE_TILE_COLUMNS << endl;
  cout << "Parameter NUM_ADDRESSABLE_REGISTERS is " << NUM_ADDRESSABLE_REGISTERS << endl;
  cout << "Parameter CORE_SCRATCHPAD_SIZE is " << CORE_SCRATCHPAD_SIZE << endl;
  cout << "Parameter IPK_FIFO_SIZE is " << IPK_FIFO_SIZE << endl;
  cout << "Parameter IPK_CACHE_SIZE is " << IPK_CACHE_SIZE << endl;
  cout << "Parameter IPK_CACHE_TAGS is " << IPK_CACHE_TAGS << endl;
  cout << "Parameter CHANNEL_MAP_SIZE is " << CHANNEL_MAP_SIZE << endl;
  cout << "Parameter MAX_IPK_SIZE is " << MAX_IPK_SIZE << endl;
  cout << "Parameter DIRECTORY_SIZE is " << DIRECTORY_SIZE << endl;
  cout << "Parameter L1_LATENCY is " << L1_LATENCY << endl;
  cout << "Parameter MEMORY_BANK_SIZE is " << MEMORY_BANK_SIZE << endl;
  cout << "Parameter MEMORY_ON_CHIP_SCRATCHPAD_DELAY is " << MEMORY_ON_CHIP_SCRATCHPAD_DELAY << endl;
  cout << "Parameter MEMORY_ON_CHIP_SCRATCHPAD_SIZE is " << MEMORY_ON_CHIP_SCRATCHPAD_SIZE << endl;
  cout << "Parameter MEMORY_ON_CHIP_SCRATCHPAD_BANKS is " << MEMORY_ON_CHIP_SCRATCHPAD_BANKS << endl;
  cout << "Parameter MEMORY_HIT_UNDER_MISS is " << MEMORY_HIT_UNDER_MISS << endl;
  cout << "Parameter NUM_RECEIVE_CHANNELS is " << NUM_RECEIVE_CHANNELS << endl;
  cout << "Parameter IN_CHANNEL_BUFFER_SIZE is " << IN_CHANNEL_BUFFER_SIZE << endl;
  cout << "Parameter OUT_CHANNEL_BUFFER_SIZE is " << OUT_CHANNEL_BUFFER_SIZE << endl;
  cout << "Parameter ROUTER_BUFFER_SIZE is " << ROUTER_BUFFER_SIZE << endl;
}

// Print parameters in a database-compatible format.
void Parameters::printParametersDbase() {
	// TODO: Add remaining parameters if required

	if (Arguments::batchMode()) {
		cout << "<@PARAM>MEMORY_BANK_SIZE:" << MEMORY_BANK_SIZE << "</@PARAM>" << endl;
		cout << "<@PARAM>MEMORY_CACHE_RANDOM_REPLACEMENT:" << MEMORY_CACHE_RANDOM_REPLACEMENT << "</@PARAM>" << endl;
		cout << "<@PARAM>MEMORY_ON_CHIP_SCRATCHPAD_DELAY:" << MEMORY_ON_CHIP_SCRATCHPAD_DELAY << "</@PARAM>" << endl;
		cout << "<@PARAM>MEMORY_ON_CHIP_SCRATCHPAD_SIZE:" << MEMORY_ON_CHIP_SCRATCHPAD_SIZE << "</@PARAM>" << endl;
		cout << "<@PARAM>MEMORY_ON_CHIP_SCRATCHPAD_BANKS:" << MEMORY_ON_CHIP_SCRATCHPAD_BANKS << "</@PARAM>" << endl;

		cout << "<@PARAM>IPK_CACHE_SIZE:" << IPK_CACHE_SIZE << "</@PARAM>" << endl;
		cout << "<@PARAM>IPK_CACHE_TAGS:" << IPK_CACHE_TAGS << "</@PARAM>" << endl;
	}
}

#define XML_LINE(name, value) "\t<" << name << ">" << value << "</" << name << ">\n"

// Print parameters in an XML format.
void Parameters::printParametersXML(std::ostream& os) {
  os << "<parameters>\n"
     << XML_LINE("cores_per_tile", CORES_PER_TILE)
     << XML_LINE("memories_per_tile", MEMS_PER_TILE)
     << XML_LINE("tile_rows", COMPUTE_TILE_ROWS)
     << XML_LINE("tile_columns", COMPUTE_TILE_COLUMNS)
     << XML_LINE("addressable_regs", NUM_ADDRESSABLE_REGISTERS)
     << XML_LINE("physical_regs", NUM_PHYSICAL_REGISTERS)
     << XML_LINE("ipk_cache_size", IPK_CACHE_SIZE)
     << XML_LINE("ipk_cache_tags", IPK_CACHE_TAGS)
     << XML_LINE("channel_map_size", CHANNEL_MAP_SIZE)
     << XML_LINE("memory_bank_size", MEMORY_BANK_SIZE)
     << "</parameters>\n";
}
