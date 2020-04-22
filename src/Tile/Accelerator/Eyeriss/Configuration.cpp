/*
 * Configuration.cpp
 *
 *  Created on: Feb 13, 2020
 *      Author: db434
 */

#include "Configuration.h"

namespace Eyeriss {

uint computeLatency(uint rows, uint columns) {
  // Eyeriss has 12 rows and 14 columns, and its scan chain has a latency of
  // 1794 cycles.
  //   * Row IDs are 4 bits on all networks (12x4x3 = 144 bits total)
  //   * Column ID width depends on datatype:
  //     * Activations:  5 bits (12x14x5 = 840 bits total)
  //     * Weights:      4 bits (12x14x4 = 672 bits total)
  //     * Partial sums: 4 bits (12x14x4 = 672 bits total)
  //   * 1 bit per PE (row?) to say whether psums come from network or neighbour
  //   * Computation configuration:
	//     * 4 bits indicating which 16 bits to use from a 32 bit psum
	//     * ???
  //   * Assignment of 25 memory banks to activations or partial sums
  // This goes way past the stated size - not sure if anything is compressed?

  // TODO: allow fields to expand if simulating a larger accelerator.
  // TODO: subtract cost of sending a conv_parameters_t to the accelerator.
  return 1794;
}

ControlFabric::ControlFabric(sc_module_name name, uint rows, uint columns) :
    LokiComponent(name),
    latency(computeLatency(rows, columns)),
    rowIDs(rows),
    columnIDs(rows, vector<uint>(columns)) {

  numFilters = numChannels = outputShift = -1;

}

// Event triggered whenever the configuration changes.
const sc_event& ControlFabric::default_event() const {
  return newConfiguration;
}

// The number of filters in this computation.
uint ControlFabric::getNumFilters() const {
  return numFilters;
}

// The number of channels in this computation.
uint ControlFabric::getNumChannels() const {
  return numChannels;
}

// The amount to right-shift results by in order to normalise them.
uint ControlFabric::getOutputShift() const {
  return outputShift;
}

// Get the tag that this row is associated with for this computation.
// Multiple row may share the same tag.
uint ControlFabric::getRowID(uint row) const {
  return rowIDs[row];
}

// Get the tag that this PE is associated with for this computation.
// Multiple PEs may share the same tag.
uint ControlFabric::getColumnID(uint row, uint column) const {
  return columnIDs[row][column];
}

// Update the configuration for a new computation.
void ControlFabric::update(uint numFilters, uint numChannels, uint outputShift) {
  this->numFilters = numFilters;
  this->numChannels = numChannels;
  this->outputShift = outputShift;

  // TODO: row/column IDs

  newConfiguration.notify(latency, sc_core::SC_NS);
}

}
