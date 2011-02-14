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

#include <string.h>

#include <iostream>
#include <string>

using std::string;
using std::cerr;
using std::endl;

//-------------------------------------------------------------------------------------------------
// General parameters
//-------------------------------------------------------------------------------------------------

int DEBUG = 0;
int TRACE = 0;

int TIMEOUT = 15000;

int BYTES_PER_WORD = 4;
int BYTES_PER_INSTRUCTION = 8;

int RETURN_CODE = 0;

//-------------------------------------------------------------------------------------------------
// Architecture size
//-------------------------------------------------------------------------------------------------

parameter CORES_PER_TILE             = 12;
parameter MEMS_PER_TILE              = 4;

parameter NUM_TILE_ROWS              = 1;
parameter NUM_TILE_COLUMNS           = 1;

//-------------------------------------------------------------------------------------------------
// Memory
//-------------------------------------------------------------------------------------------------

parameter NUM_ADDRESSABLE_REGISTERS  = 32;
parameter NUM_PHYSICAL_REGISTERS     = 64;
parameter IPK_FIFO_SIZE              = 8;
parameter IPK_CACHE_SIZE             = 1024;
parameter MEMORY_SIZE                = 10000000;
parameter CONCURRENT_MEM_OPS         = 1;//NUM_CLUSTER_INPUTS;

parameter CHANNEL_MAP_SIZE           = 4;

parameter MAX_IPK_SIZE               = 8; // Must be <= buffer size (wormhole)

//-------------------------------------------------------------------------------------------------
// Shared L1 cache subsystem
//-------------------------------------------------------------------------------------------------

parameter ENABLE_SHARED_L1_CACHE_SUBSYSTEM			= 0;

parameter SHARED_L1_CACHE_CHANNELS					= 12;
parameter SHARED_L1_CACHE_INTERFACE_QUEUE_DEPTH		= 16;

parameter SHARED_L1_CACHE_BANKS						= 1;
parameter SHARED_L1_CACHE_SETS_PER_BANK				= 8192;
parameter SHARED_L1_CACHE_ASSOCIATIVITY				= 1;
parameter SHARED_L1_CACHE_LINE_SIZE					= 4;

parameter SHARED_L1_CACHE_SEQUENTIAL_SEARCH			= 0;
parameter SHARED_L1_CACHE_RANDOM_REPLACEMENT		= 1;

parameter SHARED_L1_CACHE_MEMORY_QUEUE_DEPTH		= 16;
parameter SHARED_L1_CACHE_MEMORY_DELAY_CYCLES		= 100;

//-------------------------------------------------------------------------------------------------
// Network
//-------------------------------------------------------------------------------------------------

parameter NUM_RECEIVE_CHANNELS       = 2;
parameter NUM_MEMORY_INPUTS          = 12; // To allow SIMD experiments (not realistic)
parameter CHANNEL_END_BUFFER_SIZE    = 8;
parameter ROUTER_BUFFER_SIZE         = 4;
parameter NETWORK_BUFFER_SIZE        = 4;

parameter WORMHOLE_ROUTING           = 0; // Has negative effect on performance

//-------------------------------------------------------------------------------------------------
// Run-time parameter management
//-------------------------------------------------------------------------------------------------

void Parameters::parseParameter(const string &name, const string &value) {
	const char* cName = name.c_str();
	int nValue = StringManipulation::strToInt(value);

	if (strcasecmp(cName, "CORES_PER_TILE") == 0)
		CORES_PER_TILE = nValue;
	else if (strcasecmp(cName, "MEMS_PER_TILE") == 0)
		MEMS_PER_TILE = nValue;
	else if (strcasecmp(cName, "NUM_TILE_ROWS") == 0)
		NUM_TILE_ROWS = nValue;
	else if (strcasecmp(cName, "NUM_TILE_COLUMNS") == 0)
		NUM_TILE_COLUMNS = nValue;
	else if (strcasecmp(cName, "NUM_ADDRESSABLE_REGISTERS") == 0)
		NUM_ADDRESSABLE_REGISTERS = nValue;
	else if (strcasecmp(cName, "NUM_PHYSICAL_REGISTERS") == 0)
		NUM_PHYSICAL_REGISTERS = nValue;
	else if (strcasecmp(cName, "IPK_FIFO_SIZE") == 0)
		IPK_FIFO_SIZE = nValue;
	else if (strcasecmp(cName, "IPK_CACHE_SIZE") == 0)
		IPK_CACHE_SIZE = nValue;
	else if (strcasecmp(cName, "MEMORY_SIZE") == 0)
		MEMORY_SIZE = nValue;
	else if (strcasecmp(cName, "CONCURRENT_MEM_OPS") == 0)
		CONCURRENT_MEM_OPS = nValue;
	else if (strcasecmp(cName, "CHANNEL_MAP_SIZE") == 0)
		CHANNEL_MAP_SIZE = nValue;
	else if (strcasecmp(cName, "MAX_IPK_SIZE") == 0)
		MAX_IPK_SIZE = nValue;
	else if (strcasecmp(cName, "ENABLE_SHARED_L1_CACHE_SUBSYSTEM") == 0)
		ENABLE_SHARED_L1_CACHE_SUBSYSTEM = nValue;
	else if (strcasecmp(cName, "SHARED_L1_CACHE_CHANNELS") == 0)
		SHARED_L1_CACHE_CHANNELS = nValue;
	else if (strcasecmp(cName, "SHARED_L1_CACHE_INTERFACE_QUEUE_DEPTH") == 0)
		SHARED_L1_CACHE_INTERFACE_QUEUE_DEPTH = nValue;
	else if (strcasecmp(cName, "SHARED_L1_CACHE_BANKS") == 0)
		SHARED_L1_CACHE_BANKS = nValue;
	else if (strcasecmp(cName, "SHARED_L1_CACHE_SETS_PER_BANK") == 0)
		SHARED_L1_CACHE_SETS_PER_BANK = nValue;
	else if (strcasecmp(cName, "SHARED_L1_CACHE_ASSOCIATIVITY") == 0)
		SHARED_L1_CACHE_ASSOCIATIVITY = nValue;
	else if (strcasecmp(cName, "SHARED_L1_CACHE_LINE_SIZE") == 0)
		SHARED_L1_CACHE_LINE_SIZE = nValue;
	else if (strcasecmp(cName, "SHARED_L1_CACHE_SEQUENTIAL_SEARCH") == 0)
		SHARED_L1_CACHE_SEQUENTIAL_SEARCH = nValue;
	else if (strcasecmp(cName, "SHARED_L1_CACHE_RANDOM_REPLACEMENT") == 0)
		SHARED_L1_CACHE_RANDOM_REPLACEMENT = nValue;
	else if (strcasecmp(cName, "SHARED_L1_CACHE_MEMORY_QUEUE_DEPTH") == 0)
		SHARED_L1_CACHE_MEMORY_QUEUE_DEPTH = nValue;
	else if (strcasecmp(cName, "SHARED_L1_CACHE_MEMORY_DELAY_CYCLES") == 0)
		SHARED_L1_CACHE_MEMORY_DELAY_CYCLES = nValue;
	else if (strcasecmp(cName, "NUM_RECEIVE_CHANNELS") == 0)
		NUM_RECEIVE_CHANNELS = nValue;
	else if (strcasecmp(cName, "NUM_MEMORY_INPUTS") == 0)
		NUM_MEMORY_INPUTS = nValue;
	else if (strcasecmp(cName, "CHANNEL_END_BUFFER_SIZE") == 0)
		CHANNEL_END_BUFFER_SIZE = nValue;
	else if (strcasecmp(cName, "ROUTER_BUFFER_SIZE") == 0)
		ROUTER_BUFFER_SIZE = nValue;
	else if (strcasecmp(cName, "NETWORK_BUFFER_SIZE") == 0)
		NETWORK_BUFFER_SIZE = nValue;
	else if (strcasecmp(cName, "WORMHOLE_ROUTING") == 0)
		WORMHOLE_ROUTING = nValue;
	else {
		cerr << "Encountered invalid parameter in settings file" << endl;
		throw std::exception();
	}
}
