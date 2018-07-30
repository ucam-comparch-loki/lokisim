/*
 * MemoryChannelAdjuster.h
 * 
 * Memory mappings can cover a group of memory banks. This module determines the
 * single bank to access for a single request, based on other relevant
 * information.
 *
 * TODO: Use this component in FetchStage and ExecuteStage.
 *
 *  Created on: 30 Jul 2018
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORYCHANNELADJUSTER_H_
#define SRC_TILE_MEMORYCHANNELADJUSTER_H_

class MemoryChannelAdjuster {

//============================================================================//
// Constructors and destructors
//============================================================================//
  
public:
  
  MemoryChannelAdjuster(ComponentID id);


//============================================================================//
// Methods
//============================================================================//
  
public:
  
  // Create a network address based on the address of the data to be accessed
  // and the memory mapping to be used.
  ChannelID getMapping(const MemoryOpcode operation,
                       const uint32_t payload,
                       const ChannelMapEntry::MemoryChannel mapping);

private:

  // Determine whether this flit holds a memory address or a payload.
  bool containsAddress(const MemoryOpcode operation) const;


//============================================================================//
// Local state
//============================================================================//
  
private:
  
  // This component's identifier. Used to help fill in the address of components
  // on the same tile.
  const ComponentID id;

  // The same adjustment needs to be applied to all flits in a packet, but only
  // the first flit contains the necessary information to compute the
  // adjustment. Store it here for all other flits.
  uint32_t previousOffset;
  
  
};

#endif /* SRC_TILE_MEMORYCHANNELADJUSTER_H_ */
