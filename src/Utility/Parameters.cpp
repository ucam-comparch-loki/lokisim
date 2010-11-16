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

extern parameter CLUSTERS_PER_TILE          = 4;
extern parameter MEMS_PER_TILE              = 4;

extern parameter NUM_TILE_ROWS              = 2;
extern parameter NUM_TILE_COLUMNS           = 1;

extern parameter MAX_IPK_SIZE               = 8;

extern parameter BYTES_PER_WORD             = 4;

// Memory
extern parameter NUM_ADDRESSABLE_REGISTERS  = 32;
extern parameter NUM_PHYSICAL_REGISTERS     = 64;
extern parameter IPK_FIFO_SIZE              = 8;
extern parameter IPK_CACHE_SIZE             = 64;
extern parameter MEMORY_SIZE                = 2048;

extern parameter CHANNEL_MAP_SIZE           = 8;

// Network
extern parameter NUM_RECEIVE_CHANNELS       = 2;
extern parameter NUM_SEND_CHANNELS          = CHANNEL_MAP_SIZE;
extern parameter CHANNEL_END_BUFFER_SIZE    = 4;
extern parameter ROUTER_BUFFER_SIZE         = 4;
extern parameter NETWORK_BUFFER_SIZE        = 4;

// Combinations of other parameters
extern parameter NUM_CLUSTER_INPUTS         = 2 + NUM_RECEIVE_CHANNELS;
extern parameter NUM_CLUSTER_OUTPUTS        = NUM_SEND_CHANNELS;
extern parameter COMPONENTS_PER_TILE        = CLUSTERS_PER_TILE + MEMS_PER_TILE;
extern parameter NUM_TILES                  = NUM_TILE_ROWS * NUM_TILE_COLUMNS;

extern parameter NUM_COMPONENTS             = NUM_TILES * COMPONENTS_PER_TILE;
extern parameter TOTAL_INPUTS               = NUM_COMPONENTS * NUM_CLUSTER_INPUTS;
extern parameter TOTAL_OUTPUTS              = NUM_COMPONENTS * NUM_CLUSTER_OUTPUTS;
