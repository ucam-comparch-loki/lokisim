/*
 * BandwidthMonitor.h
 *
 * Component to keep track of how many events have taken place each clock cycle.
 *
 *  Created on: 1 May 2019
 *      Author: db434
 */

#ifndef SRC_NETWORK_BANDWIDTHMONITOR_H_
#define SRC_NETWORK_BANDWIDTHMONITOR_H_

#include <assert.h>
#include "../Utility/Instrumentation.h"
#include "NetworkTypes.h"

class BandwidthMonitor {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  BandwidthMonitor(bandwidth_t maxBandwidth) : bandwidth(maxBandwidth) {
    lastEventTime = 0;
    eventsThisCycle = 0;
  }

//============================================================================//
// Methods
//============================================================================//

public:

  // Check whether any bandwidth is available.
  bool bandwidthAvailable() const {
    return (eventsThisCycle < bandwidth) ||
           (Instrumentation::currentCycle() != lastEventTime);
  }

  // Record that an event took place. Uses one unit of bandwidth for one cycle.
  void recordEvent() {
    assert(bandwidthAvailable());

    if (Instrumentation::currentCycle() != lastEventTime) {
      lastEventTime = Instrumentation::currentCycle();
      eventsThisCycle = 1;
    }
    else
      eventsThisCycle++;
  }

//============================================================================//
// Local state
//============================================================================//

private:

  // The maximum number of events that can happen in one cycle.
  const bandwidth_t bandwidth;

  // The cycle when the last event took place.
  cycle_count_t lastEventTime;

  // The number of events which have taken place this cycle.
  bandwidth_t eventsThisCycle;


};

#endif /* SRC_NETWORK_BANDWIDTHMONITOR_H_ */
