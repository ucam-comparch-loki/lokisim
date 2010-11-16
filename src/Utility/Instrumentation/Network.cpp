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

void Network::traffic(ComponentID startID, ComponentID endID) {
//  totalDistance += distance;
//
//  int rounded = round(distance);
//  commDistances.increment(rounded);
//
//  if(rounded>maxDistance) maxDistance = rounded;

  producers.increment(startID);
  consumers.increment(endID);
}

void Network::printStats() {
  if(producers.numEvents() > 0) {

    double averageDist = (double)totalDistance/commDistances.numEvents();

    cout <<
      "Network:" << endl <<
      "  Total words sent: " << commDistances.numEvents() << endl <<
      "  Average distance: " << averageDist << endl;

    cout << "  Traffic distribution:" << endl <<
            "    Component\tProduced\tConsumed" << endl;

    for(ComponentID i=0; i<NUM_COMPONENTS; i++) {
      if(producers[i]>0 || consumers[i]>0) {
        cout <<"    "<< i <<"\t\t"<< producers[i] <<"\t\t"<< consumers[i] <<endl;
      }
    }

  }
}
