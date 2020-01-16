/*
 * ChannelAdjuster.cpp
 *
 *  Created on: 30 Jul 2018
 *      Author: db434
 */

#include "MemoryBankSelector.h"

#include "../Memory/MemoryTypes.h"

MemoryBankSelector::MemoryBankSelector(ComponentID id) :
    id(id) {

  // Nothing

}

ChannelID MemoryBankSelector::getNetworkAddress(
    const ChannelMapEntry::MemoryChannel mapping,
    const MemoryAddr address) const {

  // An offset of n means that the base+n memory bank should be accessed, where
  // base is specified in the channel mapping.
  int offset = 0;

  if (mapping.logGroupSize > 1) {
    // TODO: I reorganised this slightly - check that it still works.
    int cacheLine = address / CACHE_LINE_BYTES;
    int maskForBanks = ((1 << mapping.logGroupSize)) - 1;
    offset = cacheLine & maskForBanks;
  }
  // else if group size == 1, offset remains zero.

  return ChannelID(id.tile.x, id.tile.y, mapping.bank + offset, mapping.channel);
}
