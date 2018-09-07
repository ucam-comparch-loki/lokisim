/*
 * ChannelMap.cpp
 *
 *  Created on: 21 Jun 2012
 *      Author: db434
 */

#include "ChannelMap.h"

#include "../../Tile/ChannelMapEntry.h"

using namespace Instrumentation;

count_t ChannelMap::operations[2];
count_t ChannelMap::popCount[2];
count_t ChannelMap::hammingDist[2];
count_t ChannelMap::bypasses = 0;
count_t ChannelMap::cyclesActive = 0;

void ChannelMap::dumpEventCounts(std::ostream& os, const chip_parameters_t& params) {
  os << xmlBegin("channel_map_table")     << "\n"
     << xmlNode("instances", params.totalCores()) << "\n"  // Do memories have them too?
     << xmlNode("active", cyclesActive)   << "\n"
     << xmlNode("w_op", operations[WR])   << "\n"
     << xmlNode("w_oc", popCount[WR])     << "\n"
     << xmlNode("w_hd", hammingDist[WR])  << "\n"
     << xmlNode("rd_op", operations[RD])  << "\n"
     << xmlNode("rd_oc", popCount[RD])    << "\n"
     << xmlNode("rd_hd", hammingDist[RD]) << "\n"
     << xmlNode("bypass", bypasses)       << "\n"
     << xmlEnd("channel_map_table")       << "\n";
}

void ChannelMap::write(const ChannelMapEntry& oldData, const ChannelMapEntry& newData) {
  if (!Instrumentation::collectingStats()) return;

  operations[WR]++;
  popCount[WR] += newData.popCount();
  hammingDist[WR] += oldData.hammingDistance(newData);
}

void ChannelMap::read(const ChannelMapEntry& oldData, const ChannelMapEntry& newData) {
  if (!Instrumentation::collectingStats()) return;

  operations[RD]++;
  popCount[RD] += newData.popCount();
  hammingDist[RD] += oldData.hammingDistance(newData);
}

void ChannelMap::activeCycle() {
  if (!Instrumentation::collectingStats()) return;

  cyclesActive++;
}

void ChannelMap::bypass() {
  if (!Instrumentation::collectingStats()) return;

  bypasses++;
}
