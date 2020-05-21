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

#include "../../Datatype/Word.h"
#include "../../LokiComponent.h"
#include "../../Network/NetworkTypes.h"
#include "AcceleratorChannel.h"
#include "Algorithm.h"
#include "ComputeUnit.h"
#include "ControlUnit.h"
#include "DMA.h"

class AcceleratorTile;
class Configuration;

// TODO: make this a parameter.
typedef int32_t dtype;

class Accelerator: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  typedef sc_port<network_sink_ifc<Word>> InPort;
  typedef sc_port<network_source_ifc<Word>> OutPort;

  ClockInput clock;

  // Number of input channels on core-to-core network.
  // Channel 0: computation parameters.
  // May eventually like to refine this so the same parameters can be reused
  // with different memory addresses.
  static const size_t numMulticastInputs = 1;

  // Connections to the cores on this tile.
  LokiVector<InPort>      iMulticast;
  OutPort                 oMulticast;

  // Connections to memory banks. Each DMA unit has its own set of ports.
  // Addressed using iMemory[dma][channel].
  LokiVector2D<InPort>    iMemory;
  LokiVector2D<OutPort>   oMemory;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(Accelerator);
  Accelerator(sc_module_name name, ComponentID id, uint numMemoryBanks,
              const accelerator_parameters_t& params);


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

  // Is this accelerator currently performing computation? The accelerator is
  // considered busy until the final result is sent to memory.
  bool isIdle() const;

private:

  // Check that the provided parameters are consistent. Display a warning for
  // any problems found, but do not exit simulation.
  void checkParameters(const accelerator_parameters_t& params);

  // Event which is triggered when computation has finished and all results
  // have reached memory.
  const sc_event& finishedComputationEvent() const;

  // Returns which channel of memory should be accessed. This determines which
  // component data is returned to.
  ChannelIndex memoryAccessChannel(ComponentID id) const;

  AcceleratorTile& parent() const;


//============================================================================//
// Local state
//============================================================================//

public:

  const ComponentID id;

private:

  // The control unit sets state throughout the accelerator so give it access.
  friend class ControlUnit;
  friend class DMA;

  DMAInput<dtype> in1, in2;
  DMAOutput<dtype> out;

  ControlUnit control;
  ComputeUnit<dtype> compute;

  CommandSignal toIn1, toIn2, toOut;

  AcceleratorChannel<dtype> toPEs1, toPEs2, fromPEs;

};

#endif /* SRC_TILE_ACCELERATOR_ACCELERATOR_H_ */
