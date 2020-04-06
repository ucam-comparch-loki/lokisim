/*
 * Network.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include "Network.h"
#include "../Parameters.h"
#include "../../Datatype/Flit.h"
#include "Stalls.h"

using namespace Instrumentation;

CounterMap<ComponentID> Network::producers;
CounterMap<ComponentID> Network::consumers;
count_t Network::arbitrations = 0;
count_t Network::arbiters = 0;
count_t Network::xbarInHD = 0;
count_t Network::xbarOutHD = 0;
count_t Network::xbarDistHD = 0;
count_t Network::mcastHD = 0;
count_t Network::mcastRepeatHD = 0;
count_t Network::globalHD = 0;

CounterMap<const char*> Network::bandwidth;

void Network::reset() {
  producers.clear();
  consumers.clear();
  arbitrations = 0;
  arbiters = 0;
  xbarInHD = 0;
  xbarOutHD = 0;
  xbarDistHD = 0;
  mcastHD = 0;
  mcastRepeatHD = 0;
  globalHD = 0;
  bandwidth.clear();
}

void Network::traffic(const ComponentID& startID, const ComponentID& endID) {
  if (!Instrumentation::collectingStats()) return;

  producers.increment(startID);
  consumers.increment(endID);
}

void Network::crossbarInput(const NetworkData& oldData, const NetworkData& newData,
                            const PortIndex input) {
  if (!Instrumentation::collectingStats()) return;

  uint hamming = hammingDistance(oldData, newData);
  uint destinationPort = newData.channelID().component.position;

  // Cores and memories are on different networks, so adjust the position of
  // memories to account for this.
  if (destinationPort >= CORES_PER_TILE)
    destinationPort -= CORES_PER_TILE;

  int distance = abs(int(input) - int(destinationPort));

  xbarInHD += hamming;
  xbarDistHD += hamming*distance;
}

void Network::crossbarOutput(const NetworkData& oldData, const NetworkData& newData) {
  if (!Instrumentation::collectingStats()) return;

  xbarOutHD += hammingDistance(oldData, newData);
}

void Network::multicastTraffic(const NetworkData& oldData, const NetworkData& newData,
                               const PortIndex input) {
  if (!Instrumentation::collectingStats()) return;

  mcastHD += hammingDistance(oldData, newData);
}

void Network::globalTraffic(const NetworkData& oldData, const NetworkData& newData) {
  if (!Instrumentation::collectingStats()) return;

  globalHD += hammingDistance(oldData, newData);
}

void Network::recordBandwidth(const char* name) {
  if (!Instrumentation::collectingStats()) return;

  bandwidth.increment(name);
}

void Network::arbitration() {
  if (!Instrumentation::collectingStats()) return;

  arbitrations++;
}

void Network::arbiterCreated() {
  arbiters++;
}

void Network::printStats(const chip_parameters_t& params) {
  if (producers.numEvents() > 0) {
    cout <<
      "Network:" << endl <<
      "  Total words sent: " << producers.numEvents() << "\n" <<
      "  Traffic distribution:\n" <<
      "    Component\tProduced\tConsumed\n";

    for (uint col = 1; col <= params.numComputeTiles.width; col++) {
      for (uint row = 1; row <= params.numComputeTiles.height; row++) {
        for (uint component=0; component<params.tile.totalComponents(); component++) {
          ComponentID id(col, row, component);
          if (producers[id] > 0 || consumers[id] > 0)
            cout <<"    "<< id <<"\t\t"<< producers[id] <<"\t\t"<< consumers[id] <<"\n";
        }
      }
    }
  }
}

void Network::printSummary(const chip_parameters_t& params) {
  using std::clog;
  using std::endl;

  clog << "\nMost saturated links:" << endl;
  cycle_count_t totalTime = cyclesStatsCollected();

  // Find the most used link.
  count_t maximum = 0;
  for (CounterMap<const char*>::iterator it = bandwidth.begin(); it != bandwidth.end(); it++) {
    if (it->second > maximum) {
      maximum = it->second;
    }
  }

  // Report on all links above 75% of the maximum usage.
  for (CounterMap<const char*>::iterator it = bandwidth.begin(); it != bandwidth.end(); it++) {
    if (it->second > (maximum * 0.75)) {
      clog << "  " << percentage(it->second, totalTime) << "\t" << it->first << endl;
    }
  }
}

void Network::dumpEventCounts(std::ostream& os, const chip_parameters_t& params) {
  os << xmlBegin("crossbar")              << "\n"
     << xmlNode("instances", params.numComputeTiles.total())   << "\n"
     << xmlNode("hd_in", xbarInHD)        << "\n"
     << xmlNode("hd_out", xbarOutHD)      << "\n"
     << xmlNode("total_dist", xbarDistHD) << "\n"
     << xmlEnd("crossbar")                << "\n"

     << xmlBegin("multicast_network")     << "\n"
     << xmlNode("instances", params.numComputeTiles.total())   << "\n"
     << xmlNode("hd", mcastHD)            << "\n"
     << xmlEnd("multicast_network")       << "\n"

     << xmlBegin("global_network")        << "\n"
     << xmlNode("instances", 1)           << "\n" // is this right?
     << xmlNode("hd", globalHD)           << "\n"
     << xmlEnd("global_network")          << "\n"

     << xmlBegin("arbiter")               << "\n"
     << xmlNode("instances", arbiters)    << "\n"
     << xmlNode("active", arbitrations)   << "\n"
     << xmlEnd("arbiter")                 << "\n"

     << xmlBegin("router")                << "\n"
     << xmlNode("instances", params.numComputeTiles.total())   << "\n"
     // TODO
     << xmlEnd("router")                  << "\n";
}
