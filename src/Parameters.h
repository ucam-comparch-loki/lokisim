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

typedef const int parameter;

extern parameter DEBUG;

extern parameter NUM_CHANNELS_BETWEEN_TILES;
extern parameter CLUSTERS_PER_TILE;
extern parameter MEMS_PER_TILE;

extern parameter NUM_TILE_ROWS;
extern parameter NUM_TILE_COLUMNS;

extern parameter MAX_IPK_SIZE;

extern parameter NUM_REGISTERS;
extern parameter IPK_FIFO_SIZE;
extern parameter IPK_CACHE_SIZE;
extern parameter MEMORY_SIZE;

extern parameter NUM_RECEIVE_CHANNELS;
extern parameter NUM_SEND_CHANNELS;
extern parameter CHANNEL_END_BUFFER_SIZE; // Different send/receive sizes?
extern parameter FLOW_CONTROL_BUFFER_SIZE;

extern parameter NUM_CLUSTER_INPUTS;
extern parameter NUM_CLUSTER_OUTPUTS;

#endif /* PARAMETERS_H_ */
