/*
 * ChannelAdjuster.cpp
 *
 *  Created on: 30 Jul 2018
 *      Author: db434
 */

#include "MemoryBankSelector.h"

#include "../Memory/MemoryTypes.h"

MemoryBankSelector::MemoryBankSelector(ComponentID id) :
    id(id),
    previousOffset(0) {

  // Nothing

}

ChannelID MemoryBankSelector::getMapping(const MemoryOpcode operation,
    const uint32_t payload,
    const ChannelMapEntry::MemoryChannel mapping) {

  // Determine whether the payload is a memory address or data.
  // A new address will need a new adjustment, while a payload should reuse the
  // adjustment from the previous address.
  bool addressFlit = containsAddress(operation);

  // An offset of n means that the base+n memory bank should be accessed, where
  // base is specified in the channel mapping.
  int offset = 0;

  if (mapping.logGroupSize > 1) {
    if (addressFlit) {
      // TODO: I reorganised this slightly - check that it still works.
      MemoryAddr address = payload;
      int cacheLine = address / CACHE_LINE_BYTES;
      int maskForBanks = ((1 << mapping.logGroupSize)) - 1;
      offset = cacheLine & maskForBanks;

      previousOffset = offset;
    }
    else {
      // It would be nice to have an assertion here to ensure that we expect
      // previousOffset to be used again, but there are some operations which
      // have no address flit at all which can confuse this.
      offset = previousOffset;
    }
  }
  // else if group size == 1, offset remains zero.

  return ChannelID(id.tile.x, id.tile.y, mapping.bank + offset, mapping.channel);
}

bool MemoryBankSelector::containsAddress(const MemoryOpcode operation) const {
  switch (operation) {
    case PAYLOAD:
    case PAYLOAD_EOP:
    case UPDATE_DIRECTORY_ENTRY:
    case UPDATE_DIRECTORY_MASK:
      return false;

    default:
      return true;
  }
}
