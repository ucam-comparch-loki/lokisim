/*
 * Loops.h
 *
 *  Created on: 16 Aug 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_LOOPS_H_
#define SRC_TILE_ACCELERATOR_LOOPS_H_


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

// Eight loops make a loop order. No loop can be repeated.
typedef Loop LoopOrder[8];

LoopOrder naive = {BATCH, GROUP, INPUT_CHANNEL, OUTPUT_CHANNEL,
                   IMAGE_X, IMAGE_Y, FILTER_X, FILTER_Y};


#endif /* SRC_TILE_ACCELERATOR_LOOPS_H_ */
