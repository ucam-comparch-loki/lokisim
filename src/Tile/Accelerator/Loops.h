/*
 * Loops.h
 *
 *  Created on: 16 Aug 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_LOOPS_H_
#define SRC_TILE_ACCELERATOR_LOOPS_H_

#include <cstdlib>
#include <vector>
using std::vector;

enum Loop {
  BATCH = 0,
  GROUP = 1,
  INPUT_CHANNEL = 2,
  OUTPUT_CHANNEL = 3,
  IMAGE_X = 4,
  IMAGE_Y = 5,
  FILTER_X = 6,
  FILTER_Y = 7
};

// Collection of loops to be nested, in order. First Loop is outermost.
// No Loop can be repeated.
typedef vector<Loop> LoopOrder;

namespace LoopOrders {
  extern LoopOrder naive;
};

// Details required to execute a single loop.
typedef struct {
  uint iterations; // Total number of iterations
  uint current;    // Current iteration
  int  in1Skip;    // Distance between elements of first data type in bytes
  int  in2Skip;    // Distance between elements of second data type in bytes
  int  outSkip;    // Distance between elements of output in bytes
} loop_t;


#endif /* SRC_TILE_ACCELERATOR_LOOPS_H_ */
