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
CounterMap<int> Network::producers;
CounterMap<int> Network::consumers;
int Network::maxDistance = 0;
double Network::totalDistance = 0;

void Network::traffic(int startID, int endID, double distance) {
  totalDistance += distance;

  int rounded = round(distance);
  commDistances.increment(rounded);

  if(rounded>maxDistance) maxDistance = rounded;

  producers.increment(startID);
  consumers.increment(endID);
}

void Network::printStats() {
  if(commDistances.numEvents() > 0) {

    double averageDist = (double)totalDistance/commDistances.numEvents();

    cout <<
      "Network:" << endl <<
      "  Total words sent: " << commDistances.numEvents() << endl <<
      "  Average distance: " << averageDist << endl;

    int numComponents = COMPONENTS_PER_TILE * NUM_TILE_ROWS * NUM_TILE_COLUMNS;

    cout << "  Traffic distribution:" << endl <<
            "    Component\tProduced\tConsumed" << endl;

    for(int i=0; i<numComponents; i++) {
      if(producers[i]>0 || consumers[i]>0) {
        cout <<"    "<< i <<"\t\t"<< producers[i] <<"\t\t"<< consumers[i] <<endl;
      }
    }

  }
}
