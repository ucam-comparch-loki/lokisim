/*
 * ChannelMap.cpp
 *
 *  Created on: 21 Jun 2012
 *      Author: db434
 */

#include "ChannelMap.h"
#include "../../TileComponents/ChannelMapEntry.h"

using namespace Instrumentation;

count_t ChannelMap::operations[2];
count_t ChannelMap::popCount[2];
count_t ChannelMap::hammingDist[2];
count_t ChannelMap::bypasses = 0;

void ChannelMap::dumpEventCounts(std::ostream& os) {
  os << xmlBegin("channel_map_table")     << "\n"
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
  operations[WR]++;
  popCount[WR] += newData.popCount();
  hammingDist[WR] += oldData.hammingDistance(newData);
}

void ChannelMap::read(const ChannelMapEntry& oldData, const ChannelMapEntry& newData) {
  operations[RD]++;
  popCount[RD] += newData.popCount();
  hammingDist[RD] += oldData.hammingDistance(newData);
}

void ChannelMap::bypass() {
  bypasses++;
}
