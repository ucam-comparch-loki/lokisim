/*
 * Latency.cpp
 *
 *  Created on: 13 Mar 2017
 *      Author: db434
 */

#include "Latency.h"
#include <iomanip>

using namespace Instrumentation;

TimerMap Latency::coreBufferedReq;
TimerMap Latency::coreSentReq;
TimerMap Latency::memoryReceivedReq;
TimerMap Latency::memoryStartedReq;
TimerMap Latency::memoryBufferedResp;
TimerMap Latency::memorySentResp;
TimerMap Latency::coreReceivedResp;

DurationMap Latency::coreOutBufferTime;
DurationMap Latency::networkToL1Time;
DurationMap Latency::l1InBufferTime;
DurationMap Latency::l1HitLatency;
DurationMap Latency::l1MissPenalty;
DurationMap Latency::l1OutBufferTime;
DurationMap Latency::l2HitLatency;
DurationMap Latency::l2MissPenalty;
DurationMap Latency::l2OutBufferTime;
DurationMap Latency::networkToCoreTime;

void Latency::reset() {
  coreBufferedReq.clear();
  coreSentReq.clear();
  memoryReceivedReq.clear();
  memoryStartedReq.clear();
  memoryBufferedResp.clear();
  memorySentResp.clear();
  coreReceivedResp.clear();

  coreOutBufferTime.clear();
  networkToL1Time.clear();
  l1InBufferTime.clear();
  l1HitLatency.clear();
  l1MissPenalty.clear();
  l2HitLatency.clear();
  l2MissPenalty.clear();
  l1OutBufferTime.clear();
  l2OutBufferTime.clear();
  networkToCoreTime.clear();
}

void Latency::start() {
  // Reset the times of events only. We don't want to record any super-long
  // latencies which span the period when stats weren't collected.
  coreBufferedReq.clear();
  coreSentReq.clear();
  memoryReceivedReq.clear();
  memoryStartedReq.clear();
  memoryBufferedResp.clear();
  memorySentResp.clear();
  coreReceivedResp.clear();
}

// Add an event to one of the DurationMaps.
void addDuration(DurationMap& map, ComponentID component, cycle_count_t duration) {
  DurationMap::iterator it = map.find(component);

  AverageDuration total;
  if (it == map.end())
    total = AverageDuration(duration, 1); // First time we've seen this component.
  else
    total = AverageDuration(duration + it->second.first, 1 + it->second.second);

  map[component] = total;
}

// Average across all components.
double averageDuration(const DurationMap& map) {
  cycle_count_t totalTime = 0;
  count_t totalEvents = 0;

  for (DurationMap::const_iterator it = map.begin(); it != map.end(); it++) {
    totalTime += it->second.first;
    totalEvents += it->second.second;
  }

  if (totalEvents == 0)
    return 0.0;
  else
    return (double)totalTime / totalEvents;
}

double totalDuration(const DurationMap& map) {
  cycle_count_t totalTime = 0;

  for (DurationMap::const_iterator it = map.begin(); it != map.end(); it++) {
    totalTime += it->second.first;
  }

  return totalTime;
}

double totalEvents(const DurationMap& map) {
  cycle_count_t events = 0;

  for (DurationMap::const_iterator it = map.begin(); it != map.end(); it++) {
    events += it->second.second;
  }

  return events;
}

// Average for one component.
double averageDuration(const DurationMap& map, ComponentID component) {
  AverageDuration stats = map.at(component);
  return (double)stats.first / stats.second;
}

void Latency::coreBufferedMemoryRequest(ComponentID core, const NetworkRequest& request) {
  if (!collectingStats())
    return;

  // HACK. The WriteStage currently receives data early so it doesn't have to
  // fight with other parts of the pipeline to request network resources.
  // Account for this by saying that the data was buffered one cycle later.
  coreBufferedReq[request.messageID()] = currentCycle() + 1;
}

void Latency::coreSentMemoryRequest(ComponentID core, const NetworkRequest& request) {
  if (!collectingStats())
      return;

  cycle_count_t now = currentCycle();
  coreSentReq[request.messageID()] = now;

  TimerMap::iterator it = coreBufferedReq.find(request.messageID());
  if (it != coreBufferedReq.end()) {
    cycle_count_t latency = now - it->second;
    addDuration(coreOutBufferTime, core, latency);
    coreBufferedReq.erase(it);
  }
}

void Latency::memoryReceivedRequest(ComponentID memory, const NetworkRequest& request) {
  if (!collectingStats())
      return;

  cycle_count_t now = currentCycle();
  memoryReceivedReq[request.messageID()] = now;

  TimerMap::iterator it = coreSentReq.find(request.messageID());
  if (it != coreSentReq.end()) {
    cycle_count_t latency = now - it->second;
    addDuration(networkToL1Time, memory, latency);
    coreSentReq.erase(it);
  }
}

void Latency::memoryStartedRequest(ComponentID memory, const NetworkRequest& request) {
  if (!collectingStats())
      return;

  cycle_count_t now = currentCycle();
  memoryStartedReq[request.messageID()] = now;

  TimerMap::iterator it = memoryReceivedReq.find(request.messageID());
  if (it != memoryReceivedReq.end()) {
    cycle_count_t latency = now - it->second;
    addDuration(l1InBufferTime, memory, latency);
    memoryReceivedReq.erase(it);
  }
}

void Latency::memoryBufferedResult(ComponentID memory, const NetworkRequest& request,
                                   const NetworkResponse& response, bool hit, bool l1) {
  if (!collectingStats())
      return;

  cycle_count_t now = currentCycle();
  memoryBufferedResp[response.messageID()] = now;

  TimerMap::iterator it = memoryStartedReq.find(request.messageID());
  if (it != memoryStartedReq.end()) {
    cycle_count_t latency = now - it->second;

    if (hit && l1)
      addDuration(l1HitLatency, memory, latency);
    else if (!hit && l1)
      addDuration(l1MissPenalty, memory, latency);
    else if (hit && !l1)
      addDuration(l2HitLatency, memory, latency);
    else if (!hit && !l1)
      addDuration(l2MissPenalty, memory, latency);

    memoryStartedReq.erase(it);
  }
}

void Latency::memorySentResult(ComponentID memory, const NetworkResponse& response, bool l1) {
  if (!collectingStats())
      return;

  cycle_count_t now = currentCycle();
  memorySentResp[response.messageID()] = now;

  TimerMap::iterator it = memoryBufferedResp.find(response.messageID());
  if (it != memoryBufferedResp.end()) {
    cycle_count_t latency = now - it->second;

    if (l1)
      addDuration(l1OutBufferTime, memory, latency);
    else
      addDuration(l2OutBufferTime, memory, latency);

    memoryBufferedResp.erase(it);
  }
}

void Latency::coreReceivedResult(ComponentID core, const NetworkResponse& response) {
  if (!collectingStats())
      return;

  cycle_count_t now = currentCycle();
  coreReceivedResp[response.messageID()] = now;

  TimerMap::iterator it = memorySentResp.find(response.messageID());
  if (it != memorySentResp.end()) {
    cycle_count_t latency = now - it->second;
    addDuration(networkToCoreTime, core, latency);
    memorySentResp.erase(it);
  }
}

void Latency::printSummary() {
  using std::clog;

  count_t l1Hits = totalEvents(l1HitLatency);
  count_t l1Misses = totalEvents(l1MissPenalty);
  double l1HitRate = (double)l1Hits / (l1Hits + l1Misses);
  count_t l2Hits = totalEvents(l2HitLatency);
  count_t l2Misses = totalEvents(l2MissPenalty);

  clog.precision(2);
  clog << "\nMemory latency:" << endl;
  clog << "  Average across all components (in cycles):" << endl;
  clog << "    Core output queue:  " << averageDuration(coreOutBufferTime) << endl;
  clog << "    Core -> L1 network: " << averageDuration(networkToL1Time) << endl;
  clog << "    L1 input queue:     " << averageDuration(l1InBufferTime) << endl;
  clog << "    L1 hit:             " << averageDuration(l1HitLatency) <<
          "\t(" << percentage(l1Hits, l1Hits+l1Misses) << ")" << endl;
  clog << "    L1 miss:            " << averageDuration(l1MissPenalty) <<
          "\t(" << percentage(l1Misses, l1Hits+l1Misses) << ")" << endl;
  clog << "    L1 output queue:    " << averageDuration(l1OutBufferTime) << endl;

  if (l2Hits + l2Misses > 0) {
    clog << "    L2 hit:             " << averageDuration(l2HitLatency) <<
            "\t(" << percentage(l2Hits, l2Hits+l2Misses) << ")" << endl;
    clog << "    L2 miss:            " << averageDuration(l2MissPenalty) <<
            "\t(" << percentage(l2Misses, l2Hits+l2Misses) << ")" << endl;
    clog << "    L2 output queue:    " << averageDuration(l2OutBufferTime) << endl;
  }

  clog << "    L1 -> core network: " << averageDuration(networkToCoreTime) << endl;
  clog << "    Total:              " << averageDuration(coreOutBufferTime)
      + averageDuration(networkToL1Time) + averageDuration(l1InBufferTime)
      + averageDuration(l1HitLatency) * l1HitRate
      + averageDuration(l1MissPenalty) * (1 - l1HitRate)
      + averageDuration(l1OutBufferTime)
      + averageDuration(networkToCoreTime) << endl;

  clog << "  Latency per bank (in cycles):" << endl;
  fprintf(stderr, "    %8s %15s %15s %15s %15s\n", "Bank", "Input queue", "Hit", "Miss", "Output queue");
  for (uint col = 1; col <= COMPUTE_TILE_COLUMNS; col++) {
    for (uint row = 1; row <= COMPUTE_TILE_ROWS; row++) {
      for (uint bank=CORES_PER_TILE; bank<COMPONENTS_PER_TILE; bank++) {
        ComponentID id(col, row, bank);

        // Skip a bank if there are no stats recorded.
        if (l1HitLatency.find(id) == l1HitLatency.end() &&
            l1MissPenalty.find(id) == l1MissPenalty.end() &&
            l2HitLatency.find(id) == l2HitLatency.end() &&
            l2MissPenalty.find(id) == l2MissPenalty.end())
          continue;

        double inputQueue = averageDuration(l1InBufferTime, id);
        count_t hits = l1HitLatency[id].second + l2HitLatency[id].second;
        cycle_count_t hitTime = l1HitLatency[id].first + l2HitLatency[id].first;
        count_t misses = l1MissPenalty[id].second + l2MissPenalty[id].second;
        cycle_count_t missTime = l1MissPenalty[id].first + l2MissPenalty[id].first;
        count_t outputs = l1OutBufferTime[id].second + l2OutBufferTime[id].second;
        cycle_count_t outputTime = l1OutBufferTime[id].first + l2OutBufferTime[id].first;

        std::stringstream bankID, inLatency, hitLatency, missLatency, outLatency;
        bankID << id;
        inLatency << std::setprecision(2) << inputQueue;
        hitLatency << std::setprecision(2) << (double)hitTime/hits << " (" << percentage(hits, hits+misses) << ")";
        missLatency << std::setprecision(2) << (double)missTime/misses << " (" << percentage(misses, hits+misses) << ")";
        outLatency << std::setprecision(2) << (double)outputTime/outputs;

        fprintf(stderr, "    %8s %15s %15s %15s %15s\n", bankID.str().c_str(),
            inLatency.str().c_str(), hitLatency.str().c_str(),
            missLatency.str().c_str(), outLatency.str().c_str());
      }
    }
  }
}
