/*
 * ForwardCrossbar.h
 *
 * Crossbar allowing cores to send requests to memory banks on the same tile.
 * A final output is reserved for messages to cores on other tiles.
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#ifndef SRC_TILE_NETWORK_FORWARDCROSSBAR_H_
#define SRC_TILE_NETWORK_FORWARDCROSSBAR_H_

#include "../../Network/Network.h"

class ComputeTile;

class ForwardCrossbar: public Network<Word> {

//============================================================================//
// Ports
//============================================================================//

public:

// Inherited from Crossbar:
//
//  ClockInput   clock;
//
//  LokiVector<InPort>  inputs;
//  LokiVector<OutPort> outputs;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:
  ForwardCrossbar(const sc_module_name name,
                  const tile_parameters_t& params);
  virtual ~ForwardCrossbar();

//============================================================================//
// Methods
//============================================================================//

protected:
  virtual PortIndex getDestination(const ChannelID address) const;

private:
  ComputeTile& parent() const;

};

#endif /* SRC_TILE_NETWORK_FORWARDCROSSBAR_H_ */
