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

int DEBUG = 1;

int TIMEOUT = 150000;

int       BYTES_PER_WORD             = 4;

// Architecture size
parameter CORES_PER_TILE             = 12;
parameter MEMS_PER_TILE              = 4;

parameter NUM_TILE_ROWS              = 1;
parameter NUM_TILE_COLUMNS           = 1;

// Memory
parameter NUM_ADDRESSABLE_REGISTERS  = 32;
parameter NUM_PHYSICAL_REGISTERS     = 64;
parameter IPK_FIFO_SIZE              = 8;
parameter IPK_CACHE_SIZE             = 64;
parameter MEMORY_SIZE                = 2048;
parameter CONCURRENT_MEM_OPS         = 1;//NUM_CLUSTER_INPUTS;

parameter CHANNEL_MAP_SIZE           = 4;

parameter MAX_IPK_SIZE               = 8; // Must be <= buffer size (wormhole)

// Network
parameter NUM_RECEIVE_CHANNELS       = 2;
parameter NUM_SEND_CHANNELS          = CHANNEL_MAP_SIZE;
parameter NUM_MEMORY_INPUTS          = 12; // To allow SIMD experiments (not realistic)
parameter NUM_MEMORY_OUTPUTS         = NUM_MEMORY_INPUTS;
parameter CHANNEL_END_BUFFER_SIZE    = 8;
parameter ROUTER_BUFFER_SIZE         = 4;
parameter NETWORK_BUFFER_SIZE        = 4;

parameter WORMHOLE_ROUTING           = 0; // Has negative effect on performance

// Combinations of other parameters
parameter NUM_CORE_INPUTS            = 2 + NUM_RECEIVE_CHANNELS;
parameter NUM_CORE_OUTPUTS           = NUM_SEND_CHANNELS;
parameter COMPONENTS_PER_TILE        = CORES_PER_TILE + MEMS_PER_TILE;
parameter NUM_TILES                  = NUM_TILE_ROWS * NUM_TILE_COLUMNS;

parameter NUM_CORES                  = CORES_PER_TILE * NUM_TILES;
parameter NUM_MEMORIES               = MEMS_PER_TILE * NUM_TILES;
parameter NUM_COMPONENTS             = NUM_TILES * COMPONENTS_PER_TILE;

parameter INPUTS_PER_TILE            = CORES_PER_TILE * NUM_CORE_INPUTS +
                                       MEMS_PER_TILE * NUM_MEMORY_INPUTS;
parameter OUTPUTS_PER_TILE           = CORES_PER_TILE * NUM_CORE_OUTPUTS +
                                       MEMS_PER_TILE * NUM_MEMORY_OUTPUTS;
parameter TOTAL_INPUTS               = INPUTS_PER_TILE * NUM_TILES;
parameter TOTAL_OUTPUTS              = OUTPUTS_PER_TILE * NUM_TILES;
