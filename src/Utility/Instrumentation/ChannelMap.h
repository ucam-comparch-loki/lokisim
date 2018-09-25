/*
 * ChannelMap.h
 *
 *  Created on: 21 Jun 2012
 *      Author: db434
 */

#ifndef CHANNELMAP_INSTRUMENTATION_H_
#define CHANNELMAP_INSTRUMENTATION_H_

#include "InstrumentationBase.h"

class ChannelMapEntry;

namespace Instrumentation {

class ChannelMap: public InstrumentationBase {

public:

  static void write(const ChannelMapEntry& oldData, const ChannelMapEntry& newData);
  static void read(const ChannelMapEntry& oldData, const ChannelMapEntry& newData);
  static void activeCycle();

  // For use when the register file reads and writes to the same register in
  // the same clock cycle. Is it worth having this in the channel map table?
  static void bypass();

  static void dumpEventCounts(std::ostream& os, const chip_parameters_t& params);

private:

  enum CMTPorts {WR, RD};

  // Total number of operations performed on each of the ports.
  static count_t  operations[2];

  // Information about switching.
  static count_t  popCount[2], hammingDist[2];

  // Number of times data was read from a register in the same cycle as it was
  // written.
  static count_t  bypasses;

  static count_t  cyclesActive;

};

}

#endif /* CHANNELMAP_INSTRUMENTATION_H_ */
