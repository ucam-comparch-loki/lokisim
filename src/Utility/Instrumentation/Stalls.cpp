/*
 * Stalls.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include "Stalls.h"
#include "../Parameters.h"

CounterMap<int> Stalls::stalled;
CounterMap<int> Stalls::stallTimes;
int Stalls::numStalled = 0;
int Stalls::endOfExecution = 0;

// If a core is not currently stalled, it should map to this value in the
// "stalled" mapping.
const int UNSTALLED = -1;

const int NUM_CORES = CLUSTERS_PER_TILE * NUM_TILE_ROWS * NUM_TILE_COLUMNS;

void Stalls::stall(int id, int cycle) {

  // TODO: distinguish between "stalled" and "idle"

  if(stalled[id] == UNSTALLED) {
    stalled.setCount(id, cycle);
    numStalled++;

    if(numStalled >= NUM_CORES) {
      endOfExecution = cycle;
      // Call sc_stop if we can be sure that the networks aren't active either
    }
  }
}

void Stalls::unstall(int id, int cycle) {

  if(stalled[id] != UNSTALLED) {
    int stallLength = cycle - stalled[id];
    stallTimes.setCount(id, stallTimes[id] + stallLength);
    stalled.setCount(id, UNSTALLED);
    numStalled--;
  }

}

void Stalls::printStats() {

  cout << "Cluster activity:" << endl;

  for(int i=0; i<NUM_CORES; i++) {
    // Flush any remaining stall time into the CounterMap.
    unstall(i, endOfExecution);

    // Only print statistics for clusters which have seen some activity.
    if(stallTimes[i] < endOfExecution) {
      int activeCycles = endOfExecution - stallTimes[i];
      cout << "  Cluster " << i << ": " <<
          asPercentage(activeCycles, endOfExecution) << endl;
    }
  }

  cout << "Total execution time: " << endOfExecution << " cycles" << endl;

}
