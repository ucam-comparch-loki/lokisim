/*
 * Network.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include "../../Datatype/ComponentID.h"
#include "Network.h"
#include "../Parameters.h"
#include "math.h"

CounterMap<ComponentID> Network::producers;
CounterMap<ComponentID> Network::consumers;

void Network::traffic(const ComponentID& startID, const ComponentID& endID) {
  producers.increment(startID);
  consumers.increment(endID);
}

void Network::printStats() {

  if (BATCH_MODE) {
	cout << "<@GLOBAL>network_words:" << producers.numEvents() << "</@GLOBAL>" << endl;
//	cout << "<@GLOBAL>network_avg_dist:" << averageDist << "</@GLOBAL>" << endl;
//	cout << "<@GLOBAL>network_total_dist:" << totalBitDistance_ << "</@GLOBAL>" << endl;

	//TODO: Add distribution to database if required
  }

  if(producers.numEvents() > 0) {
    cout <<
      "Network:" << endl <<
      "  Total words sent: " << producers.numEvents() << "\n" <<
      "  Traffic distribution:\n" <<
      "    Component\tProduced\tConsumed\n";

    for(uint i=0; i<NUM_TILES; i++) {
      for(uint j=0; i<COMPONENTS_PER_TILE; i++) {
        ComponentID id(i, j);
        if(producers[id]>0 || consumers[id]>0)
          cout <<"    "<< i <<"\t\t"<< producers[i] <<"\t\t"<< consumers[i] <<"\n";
      }
    }
  }
}
