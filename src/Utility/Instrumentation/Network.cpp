/*
 * Network.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include "Network.h"
#include "../Parameters.h"
#include "../../Datatype/Flit.h"
#include "../../Datatype/ComponentID.h"

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

void Network::traffic(const ComponentID& startID, const ComponentID& endID) {
  producers.increment(startID);
  consumers.increment(endID);
}

void Network::crossbarInput(const DataType& oldData, const DataType& newData,
                            const PortIndex input) {
  uint hamming = hammingDistance(oldData, newData);
  uint destinationPort = newData.channelID().getPosition();

  // Cores and memories are on different networks, so adjust the position of
  // memories to account for this.
  if (destinationPort >= CORES_PER_TILE)
    destinationPort -= CORES_PER_TILE;

  int distance = abs(input - destinationPort);

  xbarInHD += hamming;
  xbarDistHD += hamming*distance;
}

void Network::crossbarOutput(const DataType& oldData, const DataType& newData) {
  xbarOutHD += hammingDistance(oldData, newData);
}

void Network::multicastTraffic(const DataType& oldData, const DataType& newData,
                               const PortIndex input) {
  mcastHD += hammingDistance(oldData, newData);
}

void Network::globalTraffic(const DataType& oldData, const DataType& newData) {
  globalHD += hammingDistance(oldData, newData);
}

void Network::arbitration() {
  arbitrations++;
}

void Network::arbiterCreated() {
  arbiters++;
}

void Network::printStats() {

  if (BATCH_MODE) {
	cout << "<@GLOBAL>network_words:" << producers.numEvents() << "</@GLOBAL>" << endl;
  }

  if(producers.numEvents() > 0) {
    cout <<
      "Network:" << endl <<
      "  Total words sent: " << producers.numEvents() << "\n" <<
      "  Traffic distribution:\n" <<
      "    Component\tProduced\tConsumed\n";

    for(uint i=0; i<NUM_TILES; i++) {
      for(uint j=0; i<COMPONENTS_PER_TILE; i++) {
        ComponentID id(i, j);
        if(producers[id]>0 || consumers[id]>0)
          cout <<"    "<< i <<"\t\t"<< producers[i] <<"\t\t"<< consumers[i] <<"\n";
      }
    }
  }
}

void Network::dumpEventCounts(std::ostream& os) {
  os << xmlBegin("crossbar")              << "\n"
     << xmlNode("instances", NUM_TILES)   << "\n"
     << xmlNode("hd_in", xbarInHD)        << "\n"
     << xmlNode("hd_out", xbarOutHD)      << "\n"
     << xmlNode("total_dist", xbarDistHD) << "\n"
     << xmlEnd("crossbar")                << "\n"

     << xmlBegin("multicast_network")     << "\n"
     << xmlNode("instances", NUM_TILES)   << "\n"
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
     << xmlNode("instances", NUM_TILES)   << "\n"
     // TODO
     << xmlEnd("router")                  << "\n";
}
