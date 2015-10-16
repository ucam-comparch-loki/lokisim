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

#ifndef SRC_TILECOMPONENTS_MEMORY_MAGICMEMORY_H_
#define SRC_TILECOMPONENTS_MEMORY_MAGICMEMORY_H_

#include <systemc>
#include "../../Component.h"
#include "../../Datatype/Identifier.h"
#include "../../Datatype/Word.h"
#include "MemoryTypedefs.h"

class Chip;
class SimplifiedOnChipScratchpad;

class MagicMemory: public Component {

//==============================//
// Constructors and destructors
//==============================//

public:

  MagicMemory(const sc_module_name& name, SimplifiedOnChipScratchpad& mainMemory);

//==============================//
// Methods
//==============================//

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


//==============================//
// Local state
//==============================//

private:

  SimplifiedOnChipScratchpad& mainMemory;

};

#endif /* SRC_TILECOMPONENTS_MEMORY_MAGICMEMORY_H_ */
