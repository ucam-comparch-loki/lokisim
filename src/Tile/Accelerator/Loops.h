/*
 * Loops.h
 *
 *  Created on: 16 Aug 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_LOOPS_H_
#define SRC_TILE_ACCELERATOR_LOOPS_H_


enum Loop {
  BATCH,
  GROUP,
  INPUT_CHANNEL,
  OUTPUT_CHANNEL,
  IMAGE_X,
  IMAGE_Y,
  FILTER_X,
  FILTER_Y
};

// Eight loops make a loop order. No loop can be repeated.
typedef Loop LoopOrder[8];

LoopOrder naive = {BATCH, GROUP, INPUT_CHANNEL, OUTPUT_CHANNEL,
                   IMAGE_X, IMAGE_Y, FILTER_X, FILTER_Y};


#endif /* SRC_TILE_ACCELERATOR_LOOPS_H_ */
