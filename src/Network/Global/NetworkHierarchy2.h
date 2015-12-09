/*
 * NetworkHierarchy.h
 *
 * An entire network hierarchy for one particular type of data. Includes
 * connections within and between tiles.
 *
 * This network should be subclassed for each of:
 *  - Data
 *  - Credits
 *  - Memory-to-memory requests
 *  - Memory-to-memory responses
 *
 *  Created on: 1 Jul 2014
 *      Author: db434
 */

#ifndef NETWORKHIERARCHY2_H_
#define NETWORKHIERARCHY2_H_

#include <vector>

#include "../../Datatype/Identifier.h"
#include "../../Typedefs.h"
#include "../../Utility/LokiVector2D.h"
#include "../../Utility/LokiVector3D.h"
#include "../NetworkTypedefs.h"
#include "../Topologies/InstantCrossbar.h"
#include "../Topologies/Mesh.h"
#include "../WormholeMultiplexer.h"
#include "RouterDemultiplexer.h"

class InstantCrossbar;

class NetworkHierarchy2: public Component {

//==============================//
// Ports
//==============================//

public:

  ClockInput   clock;

  // One input port from each component.
  // Address using array[tile][component]
  LokiVector2D<DataInput>  iData;

  // One output port to each component.
  // Address using array[tile][component].
  LokiVector2D<DataOutput> oData;

  // One flow control signal from each input buffer of each component.
  // Address using array[tile][component][buffer].
  LokiVector3D<ReadyInput> iReady;

//==============================//
// Constructors and destructors
//==============================//

public:

  NetworkHierarchy2(const sc_module_name &name,
                    const unsigned int    sourcesPerTile,
                    const unsigned int    destinationsPerTile,
                    const unsigned int    buffersPerDestination);
  virtual ~NetworkHierarchy2();

//==============================//
// Methods
//==============================//

protected:

  // Create all signals and sub-networks, and connect up as much as possible.
  void initialise(const unsigned int sourcesPerTile,
                  const unsigned int destinationsPerTile,
                  const unsigned int buffersPerDestination);

private:

  void createNetworkToRouter(TileID tile);
  void createNetworkFromRouter(TileID tile);

//==============================//
// Components
//==============================//

protected:

  // N-to-1 and 1-to-N networks which connect all of the components to their
  // local routers.
  // Use InstantCrossbars because the router already deals with correct timing.
  vector<WormholeMultiplexer<Word>*> toRouter;
  vector<RouterDemultiplexer<Word>*> fromRouter;

  // Network containing all routers.
  Mesh globalNetwork;

  // Signals between local and global networks.
  // Address using array[column][row]
  LokiVector2D<DataSignal>    localToGlobalData,   globalToLocalData;
  LokiVector2D<ReadySignal>   localToGlobalReady,  globalToLocalReady;

};

#endif /* NETWORKHIERARCHY2_H_ */
