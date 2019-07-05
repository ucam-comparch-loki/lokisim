/*
 * DataReturn.h
 *
 * Network returning data from memory banks to cores.
 * A final input is provided for data returning from the core-to-core data
 * network.
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#ifndef SRC_TILE_NETWORK_DATARETURN_H_
#define SRC_TILE_NETWORK_DATARETURN_H_

#include "../../Network/Network.h"

class ComputeTile;

class DataReturn: public Network<Word> {

//============================================================================//
// Ports
//============================================================================//

public:

// Inherited from Network:
//
//  ClockInput clock;
//
//  LokiVector<InPort> inputs;    // One per memory + 1
//  LokiVector<OutPort> outputs;  // One per core input buffer
//                                // Numbered core*(buffers per core) + buffer

//============================================================================//
// Constructors and destructors
//============================================================================//

public:
  DataReturn(const sc_module_name name,
             const tile_parameters_t& params);
  virtual ~DataReturn();

//============================================================================//
// Methods
//============================================================================//

protected:

  virtual PortIndex getDestination(const ChannelID address) const;

private:

  ComputeTile& parent() const;

//============================================================================//
// Local state
//============================================================================//

private:

  // The number of output ports which lead to the same core.
  const uint outputsPerCore;

  // Number of cores reachable through outputs.
  const uint outputCores;

  // The number of output ports which lead to the same accelerator.
  // A little hacky at the moment: each accelerator has multiple DMAs, each of
  // which behaves as its own component. This variable actually represents
  // outputs per DMA.
  const uint outputsPerAccelerator;

};

#endif /* SRC_TILE_NETWORK_DATARETURN_H_ */
