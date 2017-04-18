/*
 * MagicMemory.h
 *
 * Interface to background memory which has no latency and is capable of
 * carrying out multiple operations each cycle.
 *
 * Note that this may change the ordering of memory operations, but is still
 * sequentially consistent.
 *
 *  Created on: 15 Oct 2015
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORY_MAGICMEMORY_H_
#define SRC_TILE_MEMORY_MAGICMEMORY_H_

#include <systemc>
#include "../Datatype/Word.h"
#include "../LokiComponent.h"
#include "../Memory/MemoryTypes.h"

class Chip;
class MainMemory;

class MagicMemory: public LokiComponent {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  MagicMemory(const sc_module_name& name, MainMemory& mainMemory);

//============================================================================//
// Methods
//============================================================================//

public:

  // Perform an operation on the data in main memory. Any results are returned
  // immediately to returnChannel.
  // Only operations with 0 to 1 payload flits are currently supported.
  void operate(MemoryOpcode opcode,
               MemoryAddr address,
               ChannelID returnChannel,
               Word data = 0);

private:

  Chip& chip() const;


//============================================================================//
// Local state
//============================================================================//

private:

  MainMemory& mainMemory;

};

#endif /* SRC_TILE_MEMORY_MAGICMEMORY_H_ */
