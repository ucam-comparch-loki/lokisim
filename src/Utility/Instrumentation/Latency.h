/*
 * Latency.h
 *
 * Record the timings of various events, and the latencies between them.
 *
 *  Created on: 13 Mar 2017
 *      Author: db434
 */

#ifndef SRC_UTILITY_INSTRUMENTATION_LATENCY_H_
#define SRC_UTILITY_INSTRUMENTATION_LATENCY_H_

#include <map>
#include "InstrumentationBase.h"

class MemoryOperation;

namespace Instrumentation {


  // Use the Flit IDs as keys, and event times as values.
  typedef std::map<uint, cycle_count_t> TimerMap;

  // Record the total time spent doing something, and the number of times that
  // thing was done.
  typedef std::pair<cycle_count_t, count_t> AverageDuration;

  // Map between ComponentIDs and the AverageDuration that they performed
  // various tasks.
  typedef std::map<ComponentID, AverageDuration> DurationMap;

  class Latency: public InstrumentationBase {

  public:

    static void reset();
    static void start();

    static void coreBufferedMemoryRequest(ComponentID core,
                                          const NetworkRequest& flit);
    static void coreSentMemoryRequest(ComponentID core,
                                      const NetworkRequest& flit);
    static void memoryReceivedRequest(ComponentID memory,
                                      const NetworkRequest& flit);
    static void memoryStartedRequest(ComponentID memory,
                                     const NetworkRequest& flit,
                                     const MemoryOperation& request);
    static void memoryBufferedResult(ComponentID memory,
                                     const MemoryOperation& request,
                                     const NetworkResponse& response,
                                     bool hit, bool l1);
    static void memorySentResult(ComponentID memory,
                                 const NetworkResponse& response, bool l1);
    static void coreReceivedResult(ComponentID core,
                                   const NetworkResponse& response);

    static void printSummary(const chip_parameters_t& params);

  private:

    // Record times of events.
    static TimerMap coreBufferedReq;
    static TimerMap coreSentReq;
    static TimerMap memoryReceivedReq;
    static TimerMap memoryStartedReq;
    static TimerMap memoryBufferedResp;
    static TimerMap memorySentResp;
    static TimerMap coreReceivedResp;

    // Record latencies of actions.
    static DurationMap coreOutBufferTime;
    static DurationMap networkToL1Time;
    static DurationMap l1InBufferTime;
    static DurationMap l1HitLatency;
    static DurationMap l1MissPenalty;
    static DurationMap l1OutBufferTime;
    static DurationMap l2HitLatency;
    static DurationMap l2MissPenalty;
    static DurationMap l2OutBufferTime;
    static DurationMap networkToCoreTime;

  };

} /* namespace Instrumentation */

#endif /* SRC_UTILITY_INSTRUMENTATION_LATENCY_H_ */
