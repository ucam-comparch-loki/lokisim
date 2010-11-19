/*
 * Parameters.h
 *
 * Contains a list of all parameters in the program. The values are held in
 * Parameters.cpp, to prevent mass re-compilation when a value is changed.
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#ifndef PARAMETERS_H_
#define PARAMETERS_H_

typedef const unsigned int parameter;

extern int DEBUG;

extern int       BYTES_PER_WORD;

// Architecture size
extern parameter CLUSTERS_PER_TILE;
extern parameter MEMS_PER_TILE;

extern parameter NUM_TILE_ROWS;
extern parameter NUM_TILE_COLUMNS;

// Memory
extern parameter NUM_ADDRESSABLE_REGISTERS;
extern parameter NUM_PHYSICAL_REGISTERS;
extern parameter IPK_FIFO_SIZE;
extern parameter IPK_CACHE_SIZE;
extern parameter MEMORY_SIZE;

extern parameter CHANNEL_MAP_SIZE;

extern parameter MAX_IPK_SIZE;

// Network
extern parameter NUM_RECEIVE_CHANNELS;
extern parameter NUM_SEND_CHANNELS;
extern parameter CHANNEL_END_BUFFER_SIZE; // Different send/receive sizes?
extern parameter ROUTER_BUFFER_SIZE;
extern parameter NETWORK_BUFFER_SIZE;

extern parameter WORMHOLE_ROUTING;

// Combinations of other parameters
extern parameter NUM_CLUSTER_INPUTS;
extern parameter NUM_CLUSTER_OUTPUTS;
extern parameter COMPONENTS_PER_TILE;
extern parameter NUM_TILES;

extern parameter NUM_COMPONENTS;
extern parameter TOTAL_INPUTS;
extern parameter TOTAL_OUTPUTS;

#endif /* PARAMETERS_H_ */
