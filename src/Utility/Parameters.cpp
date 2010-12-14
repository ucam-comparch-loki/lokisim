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
parameter CLUSTERS_PER_TILE          = 12;
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

parameter CHANNEL_MAP_SIZE           = 12;

parameter MAX_IPK_SIZE               = 8; // Must be <= buffer size (wormhole)

// Network
parameter NUM_RECEIVE_CHANNELS       = 10;
parameter NUM_SEND_CHANNELS          = CHANNEL_MAP_SIZE;
parameter CHANNEL_END_BUFFER_SIZE    = 8;
parameter ROUTER_BUFFER_SIZE         = 4;
parameter NETWORK_BUFFER_SIZE        = 4;

parameter WORMHOLE_ROUTING           = 0;

// Combinations of other parameters
parameter NUM_CLUSTER_INPUTS         = 2 + NUM_RECEIVE_CHANNELS;
parameter NUM_CLUSTER_OUTPUTS        = NUM_SEND_CHANNELS;
parameter COMPONENTS_PER_TILE        = CLUSTERS_PER_TILE + MEMS_PER_TILE;
parameter NUM_TILES                  = NUM_TILE_ROWS * NUM_TILE_COLUMNS;

parameter NUM_COMPONENTS             = NUM_TILES * COMPONENTS_PER_TILE;
parameter TOTAL_INPUTS               = NUM_COMPONENTS * NUM_CLUSTER_INPUTS;
parameter TOTAL_OUTPUTS              = NUM_COMPONENTS * NUM_CLUSTER_OUTPUTS;
