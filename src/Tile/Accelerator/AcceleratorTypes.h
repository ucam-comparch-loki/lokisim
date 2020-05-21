/*
 * AcceleratorTypes.h
 *
 *  Created on: 17 Jul 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_ACCELERATORTYPES_H_
#define SRC_TILE_ACCELERATOR_ACCELERATORTYPES_H_

#include <cstdint>
#include <ostream>
#include <systemc>
#include "../../Memory/MemoryTypes.h"
#include "../../Utility/Logging.h"

using std::ostream;
using std::vector;

// One tick is one cycle of the accelerator's functional units. This may
// differ from a clock cycle if the FUs have an initiation interval which is
// not 1, or if there is a delay somewhere, perhaps due to memory latency.
typedef uint32_t tick_t;


// A command holds all the information required to fetch a block of data for
// the FUs. A 2D data access is supported, as it must be used for at least one
// of the three data sets accessed: for 1D, set colLength to 1.
class dma_command_t {
public:

  tick_t time;            // Tick when data should be provided.
  MemoryAddr baseAddress; // Address of value to be sent to/from first PE.
  size_t rowLength;       // Number of values to fetch/store in one row.
  int    rowStride;       // Distance between values (in bytes) in row.
  size_t colLength;       // Number of values in one column.
  int    colStride;       // Distance between values (in bytes) in column.

  inline bool operator==(const dma_command_t& rhs) const {
    return rhs.time == time && rhs.baseAddress == baseAddress &&
           rhs.rowLength == rowLength && rhs.rowStride == rowStride &&
           rhs.colLength == colLength && rhs.colStride == colStride;
  }

  inline dma_command_t& operator=(const dma_command_t& rhs) {
    time = rhs.time;
    baseAddress = rhs.baseAddress;
    rowLength = rhs.rowLength;
    rowStride = rhs.rowStride;
    colLength = rhs.colLength;
    colStride = rhs.colStride;
    return *this;
  }

  inline friend void sc_trace(sc_core::sc_trace_file *tf, const dma_command_t& v, const std::string& txt) {
    // Nothing
  }

  inline friend ostream& operator<<(ostream& os, dma_command_t const &v) {
    // e.g. time: 0xBASEADDR 4x8 (0x4, 0x108)
    os << v.time << ": " << LOKI_HEX(v.baseAddress) << " " << v.rowLength << "x"
       << v.colLength << " (" << LOKI_HEX(v.rowStride) << ", "
       << LOKI_HEX(v.colStride) << ")";
    return os;
  }
};


// There is scope to compress the following structs by using smaller datatypes,
// but I believe the overhead to be negligible compared to the amount of work
// performed using the parameters.

// Location of data in memory.
typedef struct {
  // An encoded ChannelMapEntry telling where to look in the memory system.
  // This includes details such as which memory banks to access, and whether to
  // access them in cache or scratchpad modes.
  uint32_t memory_config;

  MemoryAddr address;
} memory_location_t;

// Details of how one loop iterates through the data. Each loop may iterate (or
// not) through each of the two input arrays and one output array.
// All distances are in bytes.
typedef struct {
  uint32_t in1_stride;
  uint32_t in2_stride;
  uint32_t out_stride;
} loop_iteration_t;

// All information required to specify a computation. This is the struct to be
// sent to the accelerator. Computation begins when the final parameter arrives.
// TODO: allow pieces of the struct to be reused (useful for tiled computation).
//   * Input/output locations change every tile
//   * Iteration counts usually constant, but may vary from tile to tile
//   * Loop nest is constant for a whole layer
typedef struct {
  // The accelerator will send a token to this address when computation is
  // finished.
  uint32_t notification_address;

  // Loop nest.
  uint32_t loop_count;
  vector<loop_iteration_t> loops;

  // Number of iterations of each loop.
  vector<uint32_t> iteration_counts;

  // Details of where to find the input/output data.
  memory_location_t in1;
  memory_location_t in2;
  memory_location_t out;
} lat_parameters_t;

#endif /* SRC_TILE_ACCELERATOR_ACCELERATORTYPES_H_ */
