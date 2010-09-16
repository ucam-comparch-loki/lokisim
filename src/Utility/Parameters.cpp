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

extern parameter NUM_CHANNELS_BETWEEN_TILES = 4;
extern parameter CLUSTERS_PER_TILE = 12;
extern parameter MEMS_PER_TILE = 4;

extern parameter NUM_TILE_ROWS = 1;
extern parameter NUM_TILE_COLUMNS = 1;

extern parameter MAX_IPK_SIZE = 40;

extern parameter BYTES_PER_WORD = 4;

// Memory
extern parameter NUM_ADDRESSABLE_REGISTERS = 32;
extern parameter NUM_PHYSICAL_REGISTERS = 64;
extern parameter IPK_FIFO_SIZE = 8;
extern parameter IPK_CACHE_SIZE = 64;
extern parameter MEMORY_SIZE = 2048;

extern parameter NUM_RECEIVE_CHANNELS = 2;
extern parameter NUM_SEND_CHANNELS = 2;
extern parameter CHANNEL_END_BUFFER_SIZE = 4;
extern parameter FLOW_CONTROL_BUFFER_SIZE = 4;

extern parameter CHANNEL_MAP_SIZE = 8;

// Combinations of other parameters
extern parameter NUM_CLUSTER_INPUTS = 2 + NUM_RECEIVE_CHANNELS;
extern parameter NUM_CLUSTER_OUTPUTS = 1 + NUM_SEND_CHANNELS;
extern parameter COMPONENTS_PER_TILE = CLUSTERS_PER_TILE + MEMS_PER_TILE;
extern parameter NUM_TILES = NUM_TILE_ROWS * NUM_TILE_COLUMNS;
