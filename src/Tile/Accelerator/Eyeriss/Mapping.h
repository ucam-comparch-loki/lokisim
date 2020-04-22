/*
 * Mapping.h
 *
 * Functions to help map computations to the Eyeriss fabric.
 *
 * In the original paper, these mappings are performed statically and compiled
 * into the program, but we include them here to simplify the toolflow.
 *
 * https://ieeexplore.ieee.org/document/7551407
 * https://ieeexplore.ieee.org/document/7738524
 *
 *  Created on: Feb 17, 2020
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_EYERISS_MAPPING_H_
#define SRC_TILE_ACCELERATOR_EYERISS_MAPPING_H_

#include "../AcceleratorTypes.h"

namespace Eyeriss {

// The size of PE array required for a 2D convolution, assuming unlimited PEs
// are available, and the number of these convolutions required for the whole
// computation.
struct pe_set_t {
  int width;
  int height;
  int count;  // Number of identical PE sets needed.
};

// PEs to use for one tile of computation. A single convolution may require
// multiple tiles of computation, and multiple tiles may map to the physical PE
// array simultaneously.
struct tile_t {
  int width;
  int height;
};

class Mapping {

  // Assuming unlimited PEs are available, return the size of the PE array
  // required for one 2D convolution. The Eyeriss term for this is a PE set.
  static pe_set_t mapLogical(const conv_parameters_t& params);

  // When a PE set is too large to fit on the given PE array, find the best
  // tile size, optimising for overall energy efficiency.
  static tile_t stripMine(const pe_set_t& peSet,
                          const conv_parameters_t& conv,
                          const accelerator_parameters_t& hw);

  // Map a PE set to the available PEs.
  // A PE set may need to be broken down into smaller pieces to fit it on the
  // array. Spare capacity can be utilised by duplicating a PE set. Large
  // computations may need to be split over multiple "iterations" in time.
  // The returned structure is indexed using mapping[iteration][PE set].
  static vector<vector<mapping_t>> mapPhysical(const pe_set_t& peSets,
                                               const accelerator_parameters_t& hw);

  // We have a few example mappings in the paper - check that these functions
  // give the expected result.
  static void test();

};

} // end namespace

#endif /* SRC_TILE_ACCELERATOR_EYERISS_MAPPING_H_ */
