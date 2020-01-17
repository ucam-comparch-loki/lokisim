/*
 * MemoryBankSelector.h
 *
 * Memory mappings can cover a group of memory banks. This module determines the
 * single bank to access for a single request, based on other relevant
 * information.
 *
 *  Created on: 30 Jul 2018
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORYBANKSELECTOR_H_
#define SRC_TILE_MEMORYBANKSELECTOR_H_

#include "../Datatype/Identifier.h"
#include "../Memory/MemoryTypes.h"
#include "ChannelMapEntry.h"

class MemoryBankSelector {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  MemoryBankSelector(ComponentID id);


//============================================================================//
// Methods
//============================================================================//

public:

  // Create a network address based on the address of the data to be accessed
  // and the memory mapping to be used.
  ChannelID getNetworkAddress(const ChannelMapEntry::MemoryChannel mapping,
                              const MemoryAddr address) const;

  // TODO
  // Might need a getPreviousAddress to allow multi-flit sendconfigs.
  // Would need to ensure no race conditions with fetches.

//============================================================================//
// Local state
//============================================================================//

private:

  // This component's identifier. Used to help fill in the address of components
  // on the same tile.
  const ComponentID id;


};

#endif /* SRC_TILE_MEMORYBANKSELECTOR_H_ */
