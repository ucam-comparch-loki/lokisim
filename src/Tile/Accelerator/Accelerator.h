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
#include "../../Network/ArbitratedMultiplexer.h"
#include "../../Network/NetworkTypes.h"
#include "ComputeUnit.h"
#include "ControlUnit.h"
#include "ConvolutionAlgorithm.h"
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

  ClockInput iClock;

  // Connections to the cores on this tile.
  LokiVector<DataInput>   iMulticast;
  DataOutput              oMulticast;
  LokiVector<ReadyOutput> oReadyData;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(Accelerator);
  Accelerator(sc_module_name name, ComponentID id,
              const accelerator_parameters_t& params,
              size_t numMulticastInputs);


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

  // Pass an incoming parameter from the network interface to the relevant
  // internal component.
  void receiveParameter();

  // Returns which channel of memory should be accessed. This determines which
  // component data is returned to.
  ChannelIndex memoryAccessChannel() const;

  AcceleratorTile& parent() const;


//============================================================================//
// Local state
//============================================================================//

private:

  // The control unit sets state throughout the accelerator so give it access.
  friend class ControlUnit;
  friend class DMA;

  ControlUnit control;
  DMAInput<dtype> in1, in2;
  DMAOutput<dtype> out;

  ComputeUnit<dtype> compute;

  CommandSignal toIn1, toIn2, toOut;

  LokiVector2D<sc_signal<dtype>> toPEs1, toPEs2, fromPEs;
  ReadySignal in1Ready, in2Ready, outReady;
  sc_signal<uint> inTickSig, outTickSig;

  // Multiplex down the inputs from each core to a single channel.
  ArbitratedMultiplexer<NetworkData> inputMux;
  loki_signal<NetworkData> muxOutput;
  sc_signal<bool> muxHold;
  loki_signal<uint32_t> paramSig;

};

#endif /* SRC_TILE_ACCELERATOR_ACCELERATOR_H_ */
