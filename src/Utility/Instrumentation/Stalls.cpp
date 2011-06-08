/*
 * Stalls.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include <systemc>
#include "Stalls.h"
#include "../../Datatype/ComponentID.h"
#include "../Instrumentation.h"
#include "../Parameters.h"

std::map<ComponentID, unsigned long long> Stalls::startedStalling;
std::map<ComponentID, unsigned long long> Stalls::startedIdle;

CounterMap<ComponentID> Stalls::inputStalls;
CounterMap<ComponentID> Stalls::outputStalls;
CounterMap<ComponentID> Stalls::predicateStalls;
std::map<ComponentID, int> Stalls::stallReason;

CounterMap<ComponentID> Stalls::idleTimes;
uint16_t Stalls::numStalled = 0;
uint32_t Stalls::endOfExecution = 0;

// If a core is not currently stalled, it should map to this value in the
// "stalled" mapping.
const unsigned long long UNSTALLED = -1;

void Stalls::stall(const ComponentID& id, unsigned long long cycle, int reason) {
  if(stallReason[id] == NONE) {

    stallReason[id] = reason;

    startedStalling[id] = cycle;
    numStalled++;

    if(numStalled >= NUM_COMPONENTS) {
      endOfExecution = cycle;
    }
  }
}

void Stalls::unstall(const ComponentID& id, unsigned long long cycle) {
  if(stallReason[id] != NONE) {
	  unsigned long long stallLength = cycle - startedStalling[id];

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

void Stalls::idle(const ComponentID& id, unsigned long long cycle) {
  if(startedIdle[id] == UNSTALLED || (startedIdle[id]==0 && idleTimes[id]==0)) {
    startedIdle[id] = cycle;
    numStalled++;

    if(numStalled >= NUM_COMPONENTS) {
      endOfExecution = cycle;
    }
  }
}

void Stalls::active(const ComponentID& id, unsigned long long cycle) {
  if(startedIdle[id] != UNSTALLED) {
	  unsigned long long idleLength = cycle - startedIdle[id];
    idleTimes.setCount(id, idleTimes[id] + idleLength);
    startedIdle[id] = UNSTALLED;
    numStalled--;
  }
}

void Stalls::endExecution() {
  if(numStalled < NUM_COMPONENTS)
    endOfExecution = sc_core::sc_time_stamp().to_default_time_units();
}

unsigned long long  Stalls::cyclesActive(const ComponentID& core) {
  return endOfExecution - cyclesStalled(core) - cyclesIdle(core);
}

unsigned long long  Stalls::cyclesIdle(const ComponentID& core) {
  return idleTimes[core];
}

unsigned long long  Stalls::cyclesStalled(const ComponentID& core) {
  return inputStalls[core] + outputStalls[core] + predicateStalls[core];
}

unsigned long long  Stalls::executionTime() {
  return endOfExecution;
}

void Stalls::printStats() {
  //TODO: Add information to database if required

  if(endOfExecution == 0) return;

  cout << "Cluster activity:" << endl;
  cout << "  Cluster\tActive\tIdle\tStalled (input|output|predicate)" << endl;

	for(uint i=0; i<NUM_TILES; i++) {
		for(uint j=0; j<COMPONENTS_PER_TILE; j++) {
			ComponentID id(i, j);

			// Skip over memories for now -- they are not instrumented properly.
			if(id.isMemory()) continue;

			// Flush any remaining stall/idle time into the CounterMaps.
			unstall(id, endOfExecution);
			active(id, endOfExecution);

			// Only print statistics for clusters which have seen some activity.
			if((uint)idleTimes[id] < endOfExecution) {
				int totalStalled = cyclesStalled(id);
				int activeCycles = cyclesActive(id);
				cout << "  " << id << "\t\t" <<
				percentage(activeCycles, endOfExecution) << "\t" <<
				percentage(idleTimes[id], endOfExecution) << "\t" <<
				percentage(totalStalled, endOfExecution) << "\t(" <<
				percentage(inputStalls[id], totalStalled) << "|" <<
				percentage(outputStalls[id], totalStalled) << "|" <<
				percentage(predicateStalls[id], totalStalled) << ")" << endl;
			}
		}
	}

  if (BATCH_MODE)
	cout << "<@GLOBAL>total_cycles:" << endOfExecution << "</@GLOBAL>" << endl;

  cout << "Total execution time: " << endOfExecution << " cycles" << endl;

}
