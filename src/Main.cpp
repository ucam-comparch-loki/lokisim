/*
 * Main.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "Parameters.h"
#include "Chip.h"

#include <gtest/gtest.h>
#include <systemc.h>

#define GTEST_BREAK_ON_FAILURE 1    // Switch to debug mode if a test fails

int sc_main(int argc, char* argv[]) {
  //Chip chip("chip", NUM_TILE_ROWS, NUM_TILE_COLUMNS);

  // TODO: load code and put in memories

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
