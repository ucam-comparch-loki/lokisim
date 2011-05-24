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

#include "StringManipulation.h"
#include "Parameters.h"

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

int DEBUG = 1;
int TRACE = 0;
int BATCH_MODE = 0;

int TIMEOUT = 15000000;

int BYTES_PER_WORD = 4;
int BYTES_PER_INSTRUCTION = 8;

//-------------------------------------------------------------------------------------------------
// Global variables (is there a better place to put these?)
//-------------------------------------------------------------------------------------------------

int RETURN_CODE = EXIT_SUCCESS;

//-------------------------------------------------------------------------------------------------
// Architecture size
//-------------------------------------------------------------------------------------------------

parameter CORES_PER_TILE             = 8;
parameter MEMS_PER_TILE              = 8;

parameter NUM_TILE_ROWS              = 1;
parameter NUM_TILE_COLUMNS           = 1;

//-------------------------------------------------------------------------------------------------
// Memory
//-------------------------------------------------------------------------------------------------

parameter NUM_ADDRESSABLE_REGISTERS  = 32;
parameter NUM_PHYSICAL_REGISTERS     = 64;
parameter IPK_FIFO_SIZE              = 16;
parameter IPK_CACHE_SIZE             = 64;//1024;
//parameter MEMORY_SIZE                = 2048;//8 * 1024 * 1024;
parameter CONCURRENT_MEM_OPS         = 1;//NUM_MEMORY_INPUTS;

parameter CHANNEL_MAP_SIZE           = 8;

parameter MAX_IPK_SIZE               = 8; // Must be <= buffer size (wormhole)

//-------------------------------------------------------------------------------------------------
// Configurable memory system
//-------------------------------------------------------------------------------------------------

parameter MEMORY_CHANNEL_MAP_TABLE_ENTRIES		= 16;

parameter MEMORY_CACHE_SET_COUNT				= 1024;		// 8 KB per bank
parameter MEMORY_CACHE_WAY_COUNT				= 1;
parameter MEMORY_CACHE_LINE_SIZE				= 8;

parameter MEMORY_CACHE_RANDOM_REPLACEMENT		= 1;		// 0 = Ideal LRU, 1 = Random / LFSR

parameter MEMORY_ON_CHIP_SCRATCHPAD_DELAY		= 10;
parameter MEMORY_ON_CHIP_SCRATCHPAD_SIZE		= 64 * 1024 * 1024;
parameter MEMORY_ON_CHIP_SCRATCHPAD_PORTS		= 2;

//-------------------------------------------------------------------------------------------------
// Network
//
// Note: a port is a physical connection to the network, whereas a channel is
// an address accessible through the network. There may be many channels
// accessible through each port.
//-------------------------------------------------------------------------------------------------

parameter  CORE_INPUT_PORTS         = 2;
parameter  CORE_OUTPUT_PORTS        = 1;
parameter  NUM_RECEIVE_CHANNELS     = 6;  // Register-mapped inputs only

//parameter  MEMORY_INPUT_PORTS       = 2;  // Current = input channels. Aim = 1.
//parameter  MEMORY_OUTPUT_PORTS      = 1;
//parameter  MEMORY_INPUT_CHANNELS    = 8;  // Cores per tile + some extra?
//parameter  MEMORY_OUTPUT_CHANNELS   = 8;  // Same as input channels (use #define?)

parameter CHANNEL_END_BUFFER_SIZE    = 8;
parameter ROUTER_BUFFER_SIZE         = 4;
parameter NETWORK_BUFFER_SIZE        = 4;   // Obsolete?

// TODO: make wormhole routing a per-network property, rather than a global property.
parameter WORMHOLE_ROUTING           = 0; // Has negative effect on performance

//-------------------------------------------------------------------------------------------------
// Run-time parameter management
//-------------------------------------------------------------------------------------------------

// If "name" matches the name of the parameter, set the parameter to the value.
#define SET_IF_MATCH(name, value, param) if(strcasecmp(name, #param)==0)\
  param = value

void Parameters::parseParameter(const string &name, const string &value) {
	const char* cName = name.c_str();
	int nValue = StringManipulation::strToInt(value);

	SET_IF_MATCH(cName, nValue, CORES_PER_TILE);
	else SET_IF_MATCH(cName, nValue, MEMS_PER_TILE);
	else SET_IF_MATCH(cName, nValue, NUM_TILE_ROWS);
	else SET_IF_MATCH(cName, nValue, NUM_TILE_COLUMNS);
	else SET_IF_MATCH(cName, nValue, NUM_ADDRESSABLE_REGISTERS);
	else SET_IF_MATCH(cName, nValue, NUM_PHYSICAL_REGISTERS);
	else SET_IF_MATCH(cName, nValue, IPK_FIFO_SIZE);
	else SET_IF_MATCH(cName, nValue, IPK_CACHE_SIZE);
	//else SET_IF_MATCH(cName, nValue, MEMORY_SIZE);
	else SET_IF_MATCH(cName, nValue, CONCURRENT_MEM_OPS);
	else SET_IF_MATCH(cName, nValue, CHANNEL_MAP_SIZE);
	else SET_IF_MATCH(cName, nValue, MAX_IPK_SIZE);
	/*
	else SET_IF_MATCH(cName, nValue, ENABLE_SHARED_L1_CACHE_SUBSYSTEM);
	else SET_IF_MATCH(cName, nValue, SHARED_L1_CACHE_CHANNELS);
	else SET_IF_MATCH(cName, nValue, SHARED_L1_CACHE_INTERFACE_QUEUE_DEPTH);
	else SET_IF_MATCH(cName, nValue, SHARED_L1_CACHE_BANKS);
	else SET_IF_MATCH(cName, nValue, SHARED_L1_CACHE_SETS_PER_BANK);
	else SET_IF_MATCH(cName, nValue, SHARED_L1_CACHE_ASSOCIATIVITY);
	else SET_IF_MATCH(cName, nValue, SHARED_L1_CACHE_LINE_SIZE);
	else SET_IF_MATCH(cName, nValue, SHARED_L1_CACHE_SEQUENTIAL_SEARCH);
	else SET_IF_MATCH(cName, nValue, SHARED_L1_CACHE_RANDOM_REPLACEMENT);
	else SET_IF_MATCH(cName, nValue, SHARED_L1_CACHE_MEMORY_QUEUE_DEPTH);
	else SET_IF_MATCH(cName, nValue, SHARED_L1_CACHE_MEMORY_DELAY_CYCLES);
	*/
	else SET_IF_MATCH(cName, nValue, MEMORY_CHANNEL_MAP_TABLE_ENTRIES);
	else SET_IF_MATCH(cName, nValue, MEMORY_CACHE_SET_COUNT);
	else SET_IF_MATCH(cName, nValue, MEMORY_CACHE_WAY_COUNT);
	else SET_IF_MATCH(cName, nValue, MEMORY_CACHE_LINE_SIZE);
	else SET_IF_MATCH(cName, nValue, MEMORY_CACHE_RANDOM_REPLACEMENT);
	else SET_IF_MATCH(cName, nValue, MEMORY_ON_CHIP_SCRATCHPAD_DELAY);
	else SET_IF_MATCH(cName, nValue, MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
	else SET_IF_MATCH(cName, nValue, MEMORY_ON_CHIP_SCRATCHPAD_PORTS);
	else SET_IF_MATCH(cName, nValue, NUM_RECEIVE_CHANNELS);
	//else SET_IF_MATCH(cName, nValue, MEMORY_INPUT_CHANNELS);
	//else SET_IF_MATCH(cName, nValue, MEMORY_OUTPUT_CHANNELS);
	else SET_IF_MATCH(cName, nValue, CHANNEL_END_BUFFER_SIZE);
	else SET_IF_MATCH(cName, nValue, ROUTER_BUFFER_SIZE);
	else SET_IF_MATCH(cName, nValue, NETWORK_BUFFER_SIZE);
	else SET_IF_MATCH(cName, nValue, WORMHOLE_ROUTING);
	else {
		cerr << "Encountered invalid parameter in settings file: " << name << endl;
		throw std::exception();
	}
}

void Parameters::printParameters() {
	// TODO: Add remaining parameters if required

	if (BATCH_MODE) {
		cout << "<@PARAM>MEMORY_CHANNEL_MAP_TABLE_ENTRIES:" << MEMORY_CHANNEL_MAP_TABLE_ENTRIES << "</@PARAM>" << endl;
		cout << "<@PARAM>MEMORY_CACHE_SET_COUNT:" << MEMORY_CACHE_SET_COUNT << "</@PARAM>" << endl;
		cout << "<@PARAM>MEMORY_CACHE_WAY_COUNT:" << MEMORY_CACHE_WAY_COUNT << "</@PARAM>" << endl;
		cout << "<@PARAM>MEMORY_CACHE_LINE_SIZE:" << MEMORY_CACHE_LINE_SIZE << "</@PARAM>" << endl;
		cout << "<@PARAM>MEMORY_CACHE_RANDOM_REPLACEMENT:" << MEMORY_CACHE_RANDOM_REPLACEMENT << "</@PARAM>" << endl;
		cout << "<@PARAM>MEMORY_ON_CHIP_SCRATCHPAD_DELAY:" << MEMORY_ON_CHIP_SCRATCHPAD_DELAY << "</@PARAM>" << endl;
		cout << "<@PARAM>MEMORY_ON_CHIP_SCRATCHPAD_SIZE:" << MEMORY_ON_CHIP_SCRATCHPAD_SIZE << "</@PARAM>" << endl;
		cout << "<@PARAM>MEMORY_ON_CHIP_SCRATCHPAD_PORTS:" << MEMORY_ON_CHIP_SCRATCHPAD_PORTS << "</@PARAM>" << endl;
	}
}
