/*
 * BankToL2LResponses.h
 *
 * Simple network connecting all memory banks of a tile with the L2 logic of
 * the same tile
 *
 *  Created on: 3 Apr 2019
 *      Author: db434
 */

#ifndef SRC_TILE_NETWORK_BANKTOL2LRESPONSES_H_
#define SRC_TILE_NETWORK_BANKTOL2LRESPONSES_H_

#include "../../Network/Network.h"

class BankToL2LResponses: public Network<Word> {

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

  BankToL2LResponses(const sc_module_name name, const tile_parameters_t& params);
  virtual ~BankToL2LResponses();

//============================================================================//
// Methods
//============================================================================//

protected:

  virtual PortIndex getDestination(const ChannelID address) const;
};

#endif /* SRC_TILE_NETWORK_BANKTOL2LRESPONSES_H_ */
