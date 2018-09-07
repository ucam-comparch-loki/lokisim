/*
 * IPKCache.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include "IPKCache.h"

#include <iomanip>

#include "../../Datatype/Identifier.h"
#include "../../Tile/Core/Core.h"

using namespace Instrumentation;
using std::setw;
using std::vector;

vector<struct IPKCache::CoreStats> IPKCache::perCore;
struct IPKCache::CoreStats IPKCache::total;

count_t IPKCache::tagWriteHD_ = 0;   // Total Hamming distance in written cache tags
count_t IPKCache::tagWrites_ = 0;
count_t IPKCache::tagReadHD_ = 0;    // Total Hamming distance in read cache tags
count_t IPKCache::tagsActive_ = 0;
count_t IPKCache::dataActive_ = 0;

CounterMap<MemoryAddr> IPKCache::packetsExecuted;

void IPKCache::init(const chip_parameters_t& params) {
  perCore.resize(params.totalCores());
}

void IPKCache::reset() {
  total.hits = 0;
  total.misses = 0;
  total.reads = 0;
  total.writes = 0;

  perCore.assign(perCore.size(), total);

  tagWriteHD_ = tagWrites_ = tagReadHD_ = tagsActive_ = dataActive_ = 0;

  packetsExecuted.clear();
}

void IPKCache::tagCheck(const Core& core, bool hit, const MemoryAddr tag, const MemoryAddr prevCheck) {
  if (!Instrumentation::collectingStats()) return;

  if (hit) {
    total.hits++;
    perCore[core.globalCoreIndex()].hits++;
  }
  else {
    total.misses++;
    perCore[core.globalCoreIndex()].misses++;
  }

  // Assume that all tag checks result in a packet being executed.
  // This isn't quite true, but the exceptions are expected to be rare.
  packetsExecuted.increment(tag);

  if (ENERGY_TRACE)
    tagReadHD_ += hammingDistance(tag, prevCheck);
}

void IPKCache::tagWrite(const MemoryAddr oldTag, const MemoryAddr newTag) {
  if (!Instrumentation::collectingStats()) return;

  tagWriteHD_ += hammingDistance(oldTag, newTag);
  tagWrites_++;
}

void IPKCache::tagActivity() {
  if (!Instrumentation::collectingStats()) return;

  tagsActive_++;
}

void IPKCache::read(const Core& core) {
  if (!Instrumentation::collectingStats()) return;

  total.reads++;
  perCore[core.globalCoreIndex()].reads++;
}

void IPKCache::write(const Core& core) {
  if (!Instrumentation::collectingStats()) return;

  total.writes++;
  perCore[core.globalCoreIndex()].writes++;
}

void IPKCache::dataActivity() {
  if (!Instrumentation::collectingStats()) return;

  dataActive_++;
}

count_t IPKCache::numTagChecks() {return total.hits + total.misses;}
count_t IPKCache::numHits()      {return total.hits;}
count_t IPKCache::numMisses()    {return total.misses;}
count_t IPKCache::numReads()     {return total.reads;}
count_t IPKCache::numWrites()    {return total.writes;}

void IPKCache::printStats() {
  if (numTagChecks() > 0) {
  cout <<
    "Instruction packet cache:" << "\n" <<
    "  Reads:    " << numReads() << "\n" <<
    "  Writes:   " << numWrites() << "\n" <<
    "  Tag checks: " << numTagChecks() << "\n" <<
    "    Hits:   " << numHits() << "\t(" << percentage(numHits(),numTagChecks()) << ")\n" <<
    "    Misses: " << numMisses() << "\t(" << percentage(numMisses(),numTagChecks()) << ")\n";
  }
}

void IPKCache::printSummary(const chip_parameters_t& params) {
  using std::clog;

  clog << "L0 cache activity:" << endl;
  clog << "  Total instruction reads: " << numReads() << endl;
  clog << "  Packet hit rate:         " << numHits() << "/" << numTagChecks() << " (" << percentage(numHits(),numTagChecks()) << ")" << endl;

  for (uint core = 0; core < params.totalCores(); core++) {
    struct CoreStats stats = perCore[core];
    if (stats.hits>0 || stats.misses>0 || stats.reads>0 || stats.writes>0) {
      clog << "    Core " << core << ": " << stats.hits << "/" << (stats.hits+stats.misses) << " (" << percentage(stats.hits, stats.hits+stats.misses) << ")" << endl;
    }
  }
}

void IPKCache::instructionPacketStats(std::ostream& os) {
  CounterMap<MemoryAddr>::iterator it;
  for (it = packetsExecuted.begin(); it != packetsExecuted.end(); it++) {
    os << std::setfill('0') << setw(8) << std::hex << it->first << std::dec << "\t" << it->second << endl;
  }
}

void IPKCache::dumpEventCounts(std::ostream& os, const chip_parameters_t& params) {
  // TODO: record direct-mapped/fully-associative/etc.
  os << "<ipkcache entries=\"" << params.tile.core.cache.size << "\">\n"
     << xmlNode("instances", params.totalCores()) << "\n"
     << xmlNode("active", dataActive_) << "\n"
     << xmlNode("read", numReads()) << "\n"
     << xmlNode("write", numWrites()) << "\n"
     << xmlEnd("ipkcache") << "\n";

  os << "<ipkcachetags entries=\"" << params.tile.core.cache.numTags << "\">\n"
     << xmlNode("instances", params.totalCores()) << "\n"
     << xmlNode("active", tagsActive_) << "\n"
     << xmlNode("hd", tagWriteHD_) << "\n"
     << xmlNode("tag_hd", tagReadHD_) << "\n"
     << xmlNode("write", tagWrites_) << "\n"
     << xmlNode("read", numTagChecks()) << "\n"
     << xmlEnd("ipkcachetags") << "\n";
}
