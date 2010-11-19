/*
 * Network.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include "Network.h"
#include "../Parameters.h"
#include "math.h"

CounterMap<int> Network::commDistances;
CounterMap<ComponentID> Network::producers;
CounterMap<ComponentID> Network::consumers;
int Network::maxDistance = 0;
double Network::totalDistance = 0;
double Network::totalBitDistance = 0;

void Network::traffic(ComponentID startID, ComponentID endID) {
  producers.increment(startID);
  consumers.increment(endID);
}

void Network::activity(ComponentID network, ChannelIndex source,
    ChannelIndex destination, double distance, int bitsSwitched) {
  totalDistance += distance;
  totalBitDistance += distance*bitsSwitched;

  int rounded = round(distance*10);
  commDistances.increment(rounded);

  if(rounded>maxDistance) maxDistance = rounded;

  // Still lots more information here to do things with.
}

void Network::printStats() {
  if(producers.numEvents() > 0) {

    double averageDist = (double)totalDistance/commDistances.numEvents();

    cout <<
      "Network:" << endl <<
      "  Total words sent: " << producers.numEvents() << "\n" <<
      "  Average distance: " << averageDist << "mm\n" <<
      "  Total bit-millimetres: " << totalBitDistance << "\n" <<
      "  Traffic distribution:\n" <<
      "    Component\tProduced\tConsumed\n";

    for(ComponentID i=0; i<NUM_COMPONENTS; i++) {
      if(producers[i]>0 || consumers[i]>0) {
        cout <<"    "<< i <<"\t\t"<< producers[i] <<"\t\t"<< consumers[i] <<"\n";
      }
    }

  }
}
