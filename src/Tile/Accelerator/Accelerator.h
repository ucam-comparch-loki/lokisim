/*
 * Accelerator.h
 *
 * Top-level convolution accelerator component.
 *
 *  Created on: 2 Jul 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_ACCELERATOR_H_
#define SRC_TILE_ACCELERATOR_ACCELERATOR_H_

#include "../../LokiComponent.h"

// TODO: make this a parameter.
typedef int32_t dtype;

class Accelerator: public LokiComponent {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  Accelerator(sc_module_name name, ComponentID id);


//============================================================================//
// Methods
//============================================================================//

public:

  // Magic connection from memory.
  void deliverDataInternal(const NetworkData& flit);

  // Magic connection to memory. Any results are returned immediately to
  // returnChannel.
  void magicMemoryAccess(MemoryOpcode opcode, MemoryAddr address,
                         ChannelID returnChannel, Word data = 0);

private:

  AcceleratorTile* parent() const;


//============================================================================//
// Local state
//============================================================================//

private:

  // The control unit sets state throughout the accelerator so give it access.
  friend class ControlUnit;

  ControlUnit control;
  DMAInput<dtype> in1, in2;
  DMAOutput<dtype> out;

  // TODO: ComputeUnit


};

#endif /* SRC_TILE_ACCELERATOR_ACCELERATOR_H_ */
