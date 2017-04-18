/*
 * NetworkDeadEnd.h
 *
 * Dummy component to be attached to the edge of the network. Warnings are
 * displayed if any data is received.
 *
 *  Created on: 6 Oct 2016
 *      Author: db434
 */

#ifndef SRC_NETWORK_GLOBAL_NETWORKDEADEND_H_
#define SRC_NETWORK_GLOBAL_NETWORKDEADEND_H_

#include "../../LokiComponent.h"
#include "../NetworkTypes.h"

template<class T>
class NetworkDeadEnd : public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  loki_in<T>  iData;
  loki_out<T> oData;
  ReadyInput  iReady;
  ReadyOutput oReady;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(NetworkDeadEnd);
  NetworkDeadEnd(const sc_module_name& name, ComponentID id, Direction direction) :
      LokiComponent(name, id),
      iData("iData"),
      oData("oData"),
      iReady("iReady"),
      oReady("oReady"),
      direction(direction) {

    // Allow data to be sent here so we can catch the error, rather than stall
    // forever waiting for the flow control signal.
    oReady.initialize(true);

    SC_METHOD(dataArrived);
    sensitive << iData;
    dont_initialize();

  }

//============================================================================//
// Methods
//============================================================================//

private:

  void dataArrived() {
    static const string directions[5] = {"north", "east", "south", "west", "local"};

    LOKI_WARN << "Trying to send " << directions[direction] << " from tile " << id.tile << endl;
    LOKI_WARN << "  Data: " << iData.read() << endl;
    LOKI_WARN << "  Simulating up to tile (" << TOTAL_TILE_COLUMNS-1 << "," << TOTAL_TILE_ROWS-1 << ")" << endl;
  }

//============================================================================//
// Local state
//============================================================================//

private:

  // The direction data is being sent to arrive at this dead end.
  const Direction direction;

};

#endif /* SRC_NETWORK_GLOBAL_NETWORKDEADEND_H_ */
