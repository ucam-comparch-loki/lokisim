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

// Size and shape of convolution.
typedef struct {
  uint32_t batchSize;
  uint32_t inChannels;
  uint32_t outChannels;
  uint32_t imageWidth;
  uint32_t imageHeight;
  uint32_t filterWidth;
  uint32_t filterHeight;

  // Channels (both in and out) partitioned into this many groups: default 1.
  uint32_t groups;
} conv_shape_t;


// Details of 4D activations in memory.
typedef struct {
  // An encoded ChannelMapEntry telling which memory to access.
  uint32_t memoryConfigEncoded;

  MemoryAddr address;

  // Distance (in bytes) between elements in each dimension. Negative offsets
  // are allowed (makes rotation/transpose trivial).
  int32_t  batchSkip;
  int32_t  channelSkip;
  int32_t  columnSkip;
  int32_t  rowSkip;
} activation_config_t;


// Details of 4D weights in memory.
typedef struct {
  // An encoded ChannelMapEntry telling which memory to access.
  uint32_t memoryConfigEncoded;

  MemoryAddr address;

  // Distance (in bytes) between elements in each dimension. Negative offsets
  // are allowed (makes rotation/transpose trivial).
  int32_t  inChannelSkip;
  int32_t  outChannelSkip;
  int32_t  columnSkip;
  int32_t  rowSkip;

  // Not sure if this can be removed and replaced by a channelSkip and the
  // group size. It's certainly easier for it to remain separate.
  int32_t  groupSkip;
} filter_config_t;


// All information required to specify a convolution.
typedef struct {
  // Size and shape of the computation.
  conv_shape_t shape;

  // Other details of how convolution is to be performed.
  uint32_t stride;    // Distance between filter windows: default 1.
  uint32_t dilation;  // Distance between filter elements: default 0.

  // Data position and layout in memory.
  activation_config_t input;
  activation_config_t output;
  filter_config_t     filters;
} conv_parameters_t;


typedef struct {
  size_t width;
  size_t height;
} size2d_t;

#endif /* SRC_TILE_ACCELERATOR_ACCELERATORTYPES_H_ */
