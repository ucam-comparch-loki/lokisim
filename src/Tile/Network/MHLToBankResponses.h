/*
 * MHLToBankResponses.h
 *
 * Network for sending memory responses from a tile's miss handling logic to
 * the memory banks.
 *
 *  Created on: 3 Apr 2019
 *      Author: db434
 */

#ifndef SRC_TILE_NETWORK_MHLTOBANKRESPONSES_H_
#define SRC_TILE_NETWORK_MHLTOBANKRESPONSES_H_

#include "../../Network/Network.h"

class MHLToBankResponses: public Network<Word> {

//============================================================================//
// Ports
//============================================================================//

public:

// Inherited from Network:
//
//  ClockInput clock;
//
//  LokiVector<InPort> inputs;    // One per memory
//  LokiVector<OutPort> outputs;  // Only one port

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  MHLToBankResponses(const sc_module_name name, const tile_parameters_t& params);
  virtual ~MHLToBankResponses();

//============================================================================//
// Methods
//============================================================================//

protected:

  virtual PortIndex getDestination(const ChannelID address) const;

};

#endif /* SRC_TILE_NETWORK_MHLTOBANKRESPONSES_H_ */
