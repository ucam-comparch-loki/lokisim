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
int CORE_TRACE = 0;
int MEMORY_TRACE = 0;
int ENERGY_TRACE = 0;
int SOFTWARE_TRACE = 0;
int LBT_TRACE = 0;

unsigned long long TIMEOUT = 1000000000000ULL;

int BYTES_PER_WORD = 4;

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
parameter CORE_SCRATCHPAD_SIZE       = 64;
parameter IPK_FIFO_SIZE              = 24;  // Make smaller once SIMD is sorted
parameter IPK_CACHE_SIZE             = 0;   // Set in params.txt
parameter IPK_CACHE_TAGS             = 0;   // Set in params.txt

parameter CHANNEL_MAP_SIZE           = 16 - 1; // Final entry is reserved as NULL

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

parameter CORE_INPUT_PORTS         = 4;  // 2 from cores, 2 from memories
parameter CORE_OUTPUT_PORTS        = 1;  // To cores, to memories, off tile
parameter NUM_RECEIVE_CHANNELS     = 6;  // Register-mapped inputs only

parameter IN_CHANNEL_BUFFER_SIZE   = 4;
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

  SET_IF_MATCH(cName, nValue, TIMEOUT);
  else SET_IF_MATCH(cName, nValue, CORES_PER_TILE);
	else SET_IF_MATCH(cName, nValue, MEMS_PER_TILE);
	else SET_IF_MATCH(cName, nValue, NUM_TILE_ROWS);
	else SET_IF_MATCH(cName, nValue, NUM_TILE_COLUMNS);
	else SET_IF_MATCH(cName, nValue, NUM_ADDRESSABLE_REGISTERS);
	else SET_IF_MATCH(cName, nValue, NUM_PHYSICAL_REGISTERS);
	else SET_IF_MATCH(cName, nValue, IPK_FIFO_SIZE);
  else SET_IF_MATCH(cName, nValue, IPK_CACHE_SIZE);
  else SET_IF_MATCH(cName, nValue, IPK_CACHE_TAGS);
	else SET_IF_MATCH(cName, nValue, CHANNEL_MAP_SIZE);
  else SET_IF_MATCH(cName, nValue, MAX_IPK_SIZE);
  else SET_IF_MATCH(cName, nValue, CORE_SCRATCHPAD_SIZE);
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
  else SET_IF_MATCH(cName, nValue, DEBUG);
	else {
		cerr << "Encountered unhandled parameter in settings file: " << name << endl;
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

		cout << "<@PARAM>IPK_CACHE_SIZE:" << IPK_CACHE_SIZE << "</@PARAM>" << endl;
		cout << "<@PARAM>IPK_CACHE_TAGS:" << IPK_CACHE_TAGS << "</@PARAM>" << endl;
	}
}

#define XML_LINE(name, value) "\t<" << name << ">" << value << "</" << name << ">\n"

void Parameters::printParametersXML(std::ostream& os) {
  os << "<parameters>\n"
     << XML_LINE("cores_per_tile", CORES_PER_TILE)
     << XML_LINE("memories_per_tile", MEMS_PER_TILE)
     << XML_LINE("tile_rows", NUM_TILE_ROWS)
     << XML_LINE("tile_columns", NUM_TILE_COLUMNS)
     << XML_LINE("addressable_regs", NUM_ADDRESSABLE_REGISTERS)
     << XML_LINE("physical_regs", NUM_PHYSICAL_REGISTERS)
     << XML_LINE("ipk_cache_size", IPK_CACHE_SIZE)
     << XML_LINE("ipk_cache_tags", IPK_CACHE_TAGS)
     << XML_LINE("channel_map_size", CHANNEL_MAP_SIZE)
     << XML_LINE("memory_bank_size", MEMORY_BANK_SIZE)
     << "</parameters>\n";
}
