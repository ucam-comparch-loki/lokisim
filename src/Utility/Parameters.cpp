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
int MEMORY_TRACE = 0;

int TIMEOUT = 0x7FFFFFFEL;

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
parameter IPK_FIFO_SIZE              = 20;  // Make smaller once SIMD is sorted
parameter IPK_CACHE_SIZE             = 256;//64;//1024;

parameter CHANNEL_MAP_SIZE           = 8;

parameter MAX_IPK_SIZE               = 8; // Must be <= buffer size (wormhole)

//-------------------------------------------------------------------------------------------------
// Configurable memory system
//-------------------------------------------------------------------------------------------------

parameter MEMORY_CHANNEL_MAP_TABLE_ENTRIES		= 16;

parameter MEMORY_BANK_SIZE						= 8192;		// 8 KB per bank

parameter MEMORY_CACHE_RANDOM_REPLACEMENT		= 1;		// 0 = Ideal LRU, 1 = Random / LFSR

parameter MEMORY_ON_CHIP_SCRATCHPAD_DELAY		= 20;
parameter MEMORY_ON_CHIP_SCRATCHPAD_SIZE		= 32 * 1024 * 1024;
parameter MEMORY_ON_CHIP_SCRATCHPAD_BANKS		= 4;

//-------------------------------------------------------------------------------------------------
// Network
//
// Note: a port is a physical connection to the network, whereas a channel is
// an address accessible through the network. There may be many channels
// accessible through each port.
//-------------------------------------------------------------------------------------------------

parameter CORE_INPUT_PORTS         = 2;
parameter CORE_OUTPUT_PORTS        = 3;  // To cores, to memories, off tile
parameter NUM_RECEIVE_CHANNELS     = 6;  // Register-mapped inputs only

parameter IN_CHANNEL_BUFFER_SIZE   = 20;  // Make smaller once SIMD is sorted
parameter OUT_CHANNEL_BUFFER_SIZE  = 4;
parameter ROUTER_BUFFER_SIZE       = 4;

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
	else SET_IF_MATCH(cName, nValue, CHANNEL_MAP_SIZE);
	else SET_IF_MATCH(cName, nValue, MAX_IPK_SIZE);
	else SET_IF_MATCH(cName, nValue, MEMORY_CHANNEL_MAP_TABLE_ENTRIES);
	else SET_IF_MATCH(cName, nValue, MEMORY_BANK_SIZE);
	else SET_IF_MATCH(cName, nValue, MEMORY_CACHE_RANDOM_REPLACEMENT);
	else SET_IF_MATCH(cName, nValue, MEMORY_ON_CHIP_SCRATCHPAD_DELAY);
	else SET_IF_MATCH(cName, nValue, MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
	else SET_IF_MATCH(cName, nValue, MEMORY_ON_CHIP_SCRATCHPAD_BANKS);
	else SET_IF_MATCH(cName, nValue, NUM_RECEIVE_CHANNELS);
	else SET_IF_MATCH(cName, nValue, IN_CHANNEL_BUFFER_SIZE);
  else SET_IF_MATCH(cName, nValue, OUT_CHANNEL_BUFFER_SIZE);
	else SET_IF_MATCH(cName, nValue, ROUTER_BUFFER_SIZE);
	else {
		cerr << "Encountered invalid parameter in settings file: " << name << endl;
		throw std::exception();
	}
}

void Parameters::printParameters() {
	// TODO: Add remaining parameters if required

	if (BATCH_MODE) {
		cout << "<@PARAM>MEMORY_CHANNEL_MAP_TABLE_ENTRIES:" << MEMORY_CHANNEL_MAP_TABLE_ENTRIES << "</@PARAM>" << endl;
		cout << "<@PARAM>MEMORY_BANK_SIZE:" << MEMORY_BANK_SIZE << "</@PARAM>" << endl;
		cout << "<@PARAM>MEMORY_CACHE_RANDOM_REPLACEMENT:" << MEMORY_CACHE_RANDOM_REPLACEMENT << "</@PARAM>" << endl;
		cout << "<@PARAM>MEMORY_ON_CHIP_SCRATCHPAD_DELAY:" << MEMORY_ON_CHIP_SCRATCHPAD_DELAY << "</@PARAM>" << endl;
		cout << "<@PARAM>MEMORY_ON_CHIP_SCRATCHPAD_SIZE:" << MEMORY_ON_CHIP_SCRATCHPAD_SIZE << "</@PARAM>" << endl;
		cout << "<@PARAM>MEMORY_ON_CHIP_SCRATCHPAD_BANKS:" << MEMORY_ON_CHIP_SCRATCHPAD_BANKS << "</@PARAM>" << endl;
	}
}
