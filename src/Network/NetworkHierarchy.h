/*
 * NetworkHierarchy.h
 *
 * A class responsible for organising all levels of hierarchy in the network
 * for both data and credits.
 *
 *  Created on: 1 Nov 2010
 *      Author: db434
 */

#ifndef NETWORKHIERARCHY_H_
#define NETWORKHIERARCHY_H_

#include <vector>
#include "../Component.h"
#include "Network.h"

using std::vector;

class NetworkHierarchy : public Component {

//==============================//
// Ports
//==============================//

public:

  ClockInput   clock;

  // Additional clocks which are skewed, allowing multiple clocked events
  // to happen in one cycle.
  ClockInput   fastClock, slowClock;

  // Data received from each output of each networked component.
  LokiVector<DataInput>    iData;

  // Data to each input of each component.
  LokiVector<DataOutput>   oData;

  // Flow control from components for data.
  LokiVector<ReadyInput>   iReady;


//==============================//
// Constructors and destructors
//==============================//

public:

  NetworkHierarchy(sc_module_name name);
  virtual ~NetworkHierarchy();

//==============================//
// Methods
//==============================//

public:

  // Return a pointer to the given component's local network. This allows new
  // interfaces to be tried out more quickly.
  local_net_t* getLocalNetwork(ComponentID component) const;

private:

  void makeLocalNetwork(int tileID);

//==============================//
// Components
//==============================//

private:

  vector<local_net_t*> localNetworks;

};

#endif /* NETWORKHIERARCHY_H_ */
