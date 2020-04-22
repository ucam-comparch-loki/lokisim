/*
 * Mapping.cpp
 *
 *  Created on: Feb 17, 2020
 *      Author: db434
 */

#include "Mapping.h"

namespace Eyeriss {

// I borrowed the formula for computing output sizes from PyTorch:
// https://pytorch.org/docs/stable/nn.html#torch.nn.Conv2d
int outputWidth(const conv_parameters_t& conv) {
  int w_in = conv.shape.imageWidth;
  int dilation = conv.dilation;
  int w_kernel = conv.shape.filterWidth;
  int stride = conv.stride;
  int w_out = ((w_in - dilation * (w_kernel - 1) - 1) / stride) + 1;

  return w_out;
}

int outputHeight(const conv_parameters_t& conv) {
  int h_in = conv.shape.imageHeight;
  int dilation = conv.dilation;
  int h_kernel = conv.shape.filterHeight;
  int stride = conv.stride;
  int h_out = ((h_in - dilation * (h_kernel - 1) - 1) / stride) + 1;

  return h_out;
}

pe_set_t Mapping::mapLogical(const conv_parameters_t& params) {
  pe_set_t set;

  // Height = filter height.
  set.height = params.shape.filterHeight;

  // Width = output height.
  set.width = outputHeight(params);

  // Number of PE sets = input channels x output channels x batch size.
  set.count = params.shape.inChannels * params.shape.outChannels
            * params.shape.batchSize;

  return set;
}


// Relative energy costs of different operations. It doesn't matter which scale
// is used, so long as the same values are used consistently.
struct energy_cost_t {
  double mac;       // Multiply-accumulate
  double registers; // Register file access (read or write)
  double network;   // Direct inter-PE communication (includes FIFOs)
  double cache;     // Read or write (includes FIFOs)
  double dram;      // Read or write (includes FIFOs)
};

// Compute the relative energy cost of common operations, given the hardware to
// be used.
energy_cost_t getEnergy(const accelerator_parameters_t& hw) {
  // Use the default values from the paper.
  // TODO: allow energy estimates to change based on the hardware parameters.
  energy_cost_t energy;
  energy.mac       = 1;   // 16 bit
  energy.registers = 1;   // 0.5 kB
  energy.network   = 2;
  energy.cache     = 6;   // 100 kB
  energy.dram      = 200;

  return energy;
}


// The number of times a computation pattern is repeated at each level of the
// hierarchy. e.g. The number of computations which can be done on data before
// that data is replaced.
struct reuse_t {
  int registers;
  int network;
  int cache;
  int dram;
};

// The total cost of accessing and moving read-only inputs (activations and
// weights).
// "Reuse at each level is defined as the number of times each data value is
// read from this level to its lower-cost levels during its lifetime."
double inputEnergy(const energy_cost_t& energy, const reuse_t& r) {
  // This is equation 3 from the paper:
  // https://ieeexplore.ieee.org/document/7551407 (p8/pp374)

  return energy.dram * r.dram +
      energy.cache * r.dram * r.cache +
      energy.network * r.dram * r.cache * r.network +
      energy.registers * r.dram * r.cache * r.network * r.registers;
}

// The total cost of accumulating outputs.
// "The number of accumulations at each level is defined as the number of times
// each data goes in and out of its lower-cost levels during its lifetime."
double outputEnergy(const energy_cost_t& energy, const reuse_t& r) {
  // This is equation 4 from the paper:
  // https://ieeexplore.ieee.org/document/7551407 (p9/pp375)

  // "The factor of 2 accounts for both reads and writes. Note that in this
  // calculation the accumulation of the bias term is ignored, as it has
  // negligible impact on overall energy."

  return energy.dram * (2 * r.dram - 1) +
      energy.cache * 2 * r.dram * (r.cache - 1) +
      energy.network * r.dram * r.cache * (r.network - 1) +   // No factor of 2
      energy.registers * 2 * r.dram * r.cache * r.network * (r.registers - 1);
}

tile_t Mapping::stripMine(const pe_set_t& peSet,
                          const conv_parameters_t& conv,
                          const accelerator_parameters_t& hw) {
  // Aim is to find a reuse_t for inputs, weights and outputs.
  // Constraints:
  //  * hardware parameters (PE array size, number of registers, etc.)
  //  * product of all reuse_t fields >= required amount (but only just)
  //
  // Optimise for energy, not runtime (but a pattern which maximises reuse will
  // probably also be good for performance).

  energy_cost_t energy = getEnergy(hw);

  int inputReuse = conv.shape.filterWidth * conv.shape.filterHeight
                 * conv.shape.outChannels;
  int weightReuse;
  int outputReuse;

  // Simple approach for now: brute force search across all tile sizes up to the
  // physical array size. For the sizes of array we're considering, this will
  // give only a few hundred search points, which is reasonable.
  // TODO: this isn't enough - also need to consider tiling within each memory.
  // TODO: return type isn't sufficient - return the three reuse_t?
}

some_type Mapping::mapPhysical(const pe_set_t& peSets,
                               const accelerator_parameters_t& hw) {
  // If we can't fit a PE set on the hardware, strip mine.

  // If multiple PE sets fit on the PE array simultaneously,
  //   Weights can be shared across the batch dimension (RF size)
  //   Activations can be shared across the output channels dimension (array size)
  //   Partial sums can be shared across the input channels dimension (array size)
}

void Mapping::test() {

  // https://www.rle.mit.edu/eems/wp-content/uploads/2016/11/eyeriss_jssc_2017.pdf
  // "In AlexNet, the PE sets are of size 11x55 (CONV1), 45x27 (CONV2), and
  // 3x13 (CONV3–CONV5)."

  // For a 12x14 PE array, batch size 16:
  //   The 11x55 PE set of CONV1 is strip-mined to 11x7. (2 strips at a time.)
  //   The 5x27 PE set of CONV2 is divided into two segments with dimensions
  //     5x14 and 5x13. These are mapped to the array simultaneously.
  //   The 3x13 PE set of CONV3–CONV5 can easily fit into the PE array (4 sets
  //     at a time).
  //     CONV4 and CONV5 also accumulate psums between pairs of PE sets.

  // There are also some vague comparisons between different batch and array
  // sizes. They report energy/op, which I guess we can estimate.
  // https://ieeexplore.ieee.org/document/7551407 (figures 12 + 14)

}

} // end Eyeriss namespace
