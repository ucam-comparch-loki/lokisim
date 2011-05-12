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

CounterMap<int> Network::commDistances;
CounterMap<ComponentID> Network::producers;
CounterMap<ComponentID> Network::consumers;
int Network::maxDistance = 0;
double Network::totalDistance_ = 0;
double Network::totalBitDistance_ = 0;

void Network::traffic(const ComponentID& startID, const ComponentID& endID) {
  producers.increment(startID);
  consumers.increment(endID);
}

void Network::activity(const ComponentID& network, ChannelIndex source,
    ChannelIndex destination, double distance, int bitsSwitched) {
  totalDistance_ += distance;
  totalBitDistance_ += distance*bitsSwitched;

  int rounded = round(distance*10);
  commDistances.increment(rounded);

  if(rounded>maxDistance) maxDistance = rounded;

  // Still lots more information here to do things with.
}

double Network::totalDistance() {
  return totalBitDistance_;
}

void Network::printStats() {
  if(producers.numEvents() > 0) {

    double averageDist = (double)totalDistance_/commDistances.numEvents();

    cout <<
      "Network:" << endl <<
      "  Total words sent: " << producers.numEvents() << "\n" <<
      "  Average distance: " << averageDist << "mm\n" <<
      "  Total bit-millimetres: " << totalBitDistance_ << "\n" <<
      "  Traffic distribution:\n" <<
      "    Component\tProduced\tConsumed\n";

	for(uint i=0; i<NUM_TILES; i++) {
		for(uint j=0; i<COMPONENTS_PER_TILE; i++) {
			ComponentID id(i, j);
			if(producers[id]>0 || consumers[id]>0) {
				cout <<"    "<< i <<"\t\t"<< producers[i] <<"\t\t"<< consumers[i] <<"\n";
			}
		}
	}
  }
}
