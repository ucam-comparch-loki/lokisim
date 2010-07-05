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
CounterMap<int> Stalls::idleStart;
CounterMap<int> Stalls::idleTimes;
int Stalls::numStalled = (CLUSTERS_PER_TILE-MEMS_PER_TILE) * NUM_TILES; // Messy - fix?
int Stalls::endOfExecution = 0;

// If a core is not currently stalled, it should map to this value in the
// "stalled" mapping.
const int UNSTALLED = -1;

const int NUM_CORES = (CLUSTERS_PER_TILE+MEMS_PER_TILE) * NUM_TILES;

void Stalls::stall(int id, int cycle) {
  if(stalled[id] == UNSTALLED || (stalled[id]==0 && stallTimes[id]==0)) {
    stalled.setCount(id, cycle);
    numStalled++;

    if(numStalled >= NUM_CORES) {
      endOfExecution = cycle;
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

void Stalls::idle(int id, int cycle) {
  if(idleStart[id] == UNSTALLED || (idleStart[id]==0 && idleTimes[id]==0)) {
    idleStart.setCount(id, cycle);
    numStalled++;

    if(numStalled >= NUM_CORES) {
      endOfExecution = cycle;
    }
  }
}

void Stalls::active(int id, int cycle) {
  if(idleStart[id] != UNSTALLED) {
    int idleLength = cycle - idleStart[id];
    idleTimes.setCount(id, idleTimes[id] + idleLength);
    idleStart.setCount(id, UNSTALLED);
    numStalled--;
  }
}

void Stalls::printStats() {

  cout << "Cluster activity:" << endl;
  cout << "  Cluster\tActive\tIdle\tStalled" << endl;

  for(int i=0; i<NUM_CORES; i++) {
    // Flush any remaining stall time into the CounterMap.
    unstall(i, endOfExecution);

    // Only print statistics for clusters which have seen some activity.
    if(stallTimes[i] < endOfExecution) {
      int activeCycles = endOfExecution - stallTimes[i] - idleTimes[i];
      cout << "  " << i << "\t\t\t" <<
          asPercentage(activeCycles, endOfExecution) << "\t" <<
          asPercentage(idleTimes[i], endOfExecution) << "\t" <<
          asPercentage(stallTimes[i], endOfExecution) << endl;
    }
  }

  cout << "Total execution time: " << endOfExecution << " cycles" << endl;

}
