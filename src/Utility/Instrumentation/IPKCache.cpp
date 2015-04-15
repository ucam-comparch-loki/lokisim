/*
 * IPKCache.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include "IPKCache.h"
#include "../../Datatype/ComponentID.h"

using namespace Instrumentation;
using std::vector;

vector<struct IPKCache::CoreStats> IPKCache::perCore;
struct IPKCache::CoreStats IPKCache::total;

count_t IPKCache::tagWriteHD_ = 0;   // Total Hamming distance in written cache tags
count_t IPKCache::tagWrites_ = 0;
count_t IPKCache::tagReadHD_ = 0;    // Total Hamming distance in read cache tags
count_t IPKCache::tagsActive_ = 0;
count_t IPKCache::dataActive_ = 0;

void IPKCache::init() {
  total.hits = 0;
  total.misses = 0;
  total.reads = 0;
  total.writes = 0;

  perCore.assign(NUM_CORES, total);
}

void IPKCache::tagCheck(const ComponentID& core, bool hit, const MemoryAddr tag, const MemoryAddr prevCheck) {
  if (hit) {
    total.hits++;
    perCore[core.getGlobalCoreNumber()].hits++;
  }
  else {
    total.misses++;
    perCore[core.getGlobalCoreNumber()].misses++;
  }

  if (ENERGY_TRACE)
    tagReadHD_ += hammingDistance(tag, prevCheck);
}

void IPKCache::tagWrite(const MemoryAddr oldTag, const MemoryAddr newTag) {
  tagWriteHD_ += hammingDistance(oldTag, newTag);
  tagWrites_++;
}

void IPKCache::tagActivity() {
  tagsActive_++;
}

void IPKCache::read(const ComponentID& core) {
  total.reads++;
  perCore[core.getGlobalCoreNumber()].reads++;
}

void IPKCache::write(const ComponentID& core) {
  total.writes++;
  perCore[core.getGlobalCoreNumber()].writes++;
}

void IPKCache::dataActivity() {
  dataActive_++;
}

count_t IPKCache::numTagChecks() {return total.hits + total.misses;}
count_t IPKCache::numHits()      {return total.hits;}
count_t IPKCache::numMisses()    {return total.misses;}
count_t IPKCache::numReads()     {return total.reads;}
count_t IPKCache::numWrites()    {return total.writes;}

void IPKCache::printStats() {

  if (Arguments::batchMode()) {
	cout << "<@GLOBAL>ipkcache_reads:" << numReads() << "</@GLOBAL>" << endl;
	cout << "<@GLOBAL>ipkcache_writes:" << numWrites() << "</@GLOBAL>" << endl;
	cout << "<@GLOBAL>ipkcache_tag_checks:" << numTagChecks() << "</@GLOBAL>" << endl;
	cout << "<@GLOBAL>ipkcache_hits:" << numHits() << "</@GLOBAL>" << endl;
	cout << "<@GLOBAL>ipkcache_misses:" << numMisses() << "</@GLOBAL>" << endl;
  }

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

void IPKCache::printSummary() {
  using std::clog;

  clog << "L0 cache activity:" << endl;
  clog << "  Total instruction reads: " << numReads() << endl;
  clog << "  Packet hit rate:         " << numHits() << "/" << numTagChecks() << " (" << percentage(numHits(),numTagChecks()) << ")" << endl;

  for (uint core = 0; core < NUM_CORES; core++) {
    struct CoreStats stats = perCore[core];
    if (stats.hits>0 || stats.misses>0 || stats.reads>0 || stats.writes>0) {
      clog << "    Core " << core << ": " << stats.hits << "/" << (stats.hits+stats.misses) << " (" << percentage(stats.hits, stats.hits+stats.misses) << ")" << endl;
    }
  }
}

void IPKCache::dumpEventCounts(std::ostream& os) {
  // TODO: record direct-mapped/fully-associative/etc.
  os << "<ipkcache entries=\"" << IPK_CACHE_SIZE << "\">\n"
     << xmlNode("instances", NUM_CORES) << "\n"
     << xmlNode("active", dataActive_) << "\n"
     << xmlNode("read", numReads()) << "\n"
     << xmlNode("write", numWrites()) << "\n"
     << xmlEnd("ipkcache") << "\n";

  os << "<ipkcachetags entries=\"" << IPK_CACHE_TAGS << "\">\n"
     << xmlNode("instances", NUM_CORES) << "\n"
     << xmlNode("active", tagsActive_) << "\n"
     << xmlNode("hd", tagWriteHD_) << "\n"
     << xmlNode("tag_hd", tagReadHD_) << "\n"
     << xmlNode("write", tagWrites_) << "\n"
     << xmlNode("read", numTagChecks()) << "\n"
     << xmlEnd("ipkcachetags") << "\n";
}
