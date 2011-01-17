/*
 * Stalls.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include <systemc>
#include "Stalls.h"
#include "../Parameters.h"

std::map<ComponentID, int> Stalls::startedStalling;
std::map<ComponentID, int> Stalls::startedIdle;

CounterMap<ComponentID> Stalls::inputStalls;
CounterMap<ComponentID> Stalls::outputStalls;
CounterMap<ComponentID> Stalls::predicateStalls;
std::map<ComponentID, int> Stalls::stallReason;

CounterMap<ComponentID> Stalls::idleTimes;
uint Stalls::numStalled = (CORES_PER_TILE-MEMS_PER_TILE) * NUM_TILES; // Messy - fix?
uint Stalls::endOfExecution = 0;

// If a core is not currently stalled, it should map to this value in the
// "stalled" mapping.
const int UNSTALLED = -1;

void Stalls::stall(ComponentID id, int cycle, int reason) {
  if(stallReason[id] == NONE) {

    stallReason[id] = reason;

    startedStalling[id] = cycle;
    numStalled++;

    if(numStalled >= NUM_COMPONENTS) {
      endOfExecution = cycle;
    }
  }
}

void Stalls::unstall(ComponentID id, int cycle) {
  if(stallReason[id] != NONE) {
    int stallLength = cycle - startedStalling[id];

    switch(stallReason[id]) {
      case INPUT : {
        inputStalls.setCount(id, inputStalls[id] + stallLength);
        break;
      }
      case OUTPUT : {
        outputStalls.setCount(id, outputStalls[id] + stallLength);
        break;
      }
      case PREDICATE : {
        predicateStalls.setCount(id, predicateStalls[id] + stallLength);
        break;
      }
      default : std::cerr << "Warning: Unknown stall reason." << endl;
    }

    stallReason[id] = NONE;
    numStalled--;
  }
}

void Stalls::idle(ComponentID id, int cycle) {
  if(startedIdle[id] == UNSTALLED || (startedIdle[id]==0 && idleTimes[id]==0)) {
    startedIdle[id] = cycle;
    numStalled++;

    if(numStalled >= NUM_COMPONENTS) {
      endOfExecution = cycle;
    }
  }
}

void Stalls::active(ComponentID id, int cycle) {
  if(startedIdle[id] != UNSTALLED) {
    int idleLength = cycle - startedIdle[id];
    idleTimes.setCount(id, idleTimes[id] + idleLength);
    startedIdle[id] = UNSTALLED;
    numStalled--;
  }
}

void Stalls::endExecution() {
  if(numStalled < NUM_COMPONENTS)
    endOfExecution = sc_core::sc_time_stamp().to_default_time_units();
}

void Stalls::printStats() {

  cout << "Cluster activity:" << endl;
  cout << "  Cluster\tActive\tIdle\tStalled (input|output|predicate)" << endl;

  for(ComponentID i=0; i<NUM_COMPONENTS; i++) {
    // Skip over memories for now -- they are not instrumented properly.
    if(i%COMPONENTS_PER_TILE >= CORES_PER_TILE) continue;

    // Flush any remaining stall/idle time into the CounterMaps.
    unstall(i, endOfExecution);
    active(i, endOfExecution);

    // Only print statistics for clusters which have seen some activity.
    if((uint)idleTimes[i] < endOfExecution) {
      int totalStalled = inputStalls[i] + outputStalls[i] + predicateStalls[i];
      int activeCycles = endOfExecution - totalStalled - idleTimes[i];
      cout << "  " << i << "\t\t" <<
          asPercentage(activeCycles, endOfExecution) << "\t" <<
          asPercentage(idleTimes[i], endOfExecution) << "\t" <<
          asPercentage(totalStalled, endOfExecution) << "\t(" <<
          asPercentage(inputStalls[i], totalStalled) << "|" <<
          asPercentage(outputStalls[i], totalStalled) << "|" <<
          asPercentage(predicateStalls[i], totalStalled) << ")" << endl;
    }
  }

  cout << "Total execution time: " << endOfExecution << " cycles" << endl;

}
