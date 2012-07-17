/*
 * IPKCache.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include "IPKCache.h"
#include "../../Datatype/ComponentID.h"

using namespace Instrumentation;

count_t IPKCache::numHits_ = 0;
count_t IPKCache::numMisses_ = 0;
count_t IPKCache::numReads_ = 0;
count_t IPKCache::numWrites_ = 0;
count_t IPKCache::tagHD_ = 0;   // Total Hamming distance in written cache tags
count_t IPKCache::tagWrites_ = 0;
count_t IPKCache::tagsActive_ = 0;
count_t IPKCache::dataActive_ = 0;

void IPKCache::tagCheck(const ComponentID& core, bool hit) {
  if (hit) numHits_++;
  else     numMisses_++;
}

void IPKCache::tagWrite(const MemoryAddr oldTag, const MemoryAddr newTag) {
  tagHD_ += hammingDistance(oldTag, newTag);
  tagWrites_++;
}

void IPKCache::tagActivity() {
  tagsActive_++;
}

void IPKCache::read(const ComponentID& core) {
  numReads_++;
}

void IPKCache::write(const ComponentID& core) {
  numWrites_++;
}

void IPKCache::dataActivity() {
  dataActive_++;
}

count_t IPKCache::numTagChecks() {return numHits_ + numMisses_;}
count_t IPKCache::numHits()      {return numHits_;}
count_t IPKCache::numMisses()    {return numMisses_;}
count_t IPKCache::numReads()     {return numReads_;}
count_t IPKCache::numWrites()    {return numWrites_;}

void IPKCache::printStats() {

  count_t tagChecks = numTagChecks();

  if (BATCH_MODE) {
	cout << "<@GLOBAL>ipkcache_reads:" << numReads_ << "</@GLOBAL>" << endl;
	cout << "<@GLOBAL>ipkcache_writes:" << numWrites_ << "</@GLOBAL>" << endl;
	cout << "<@GLOBAL>ipkcache_tag_checks:" << tagChecks << "</@GLOBAL>" << endl;
	cout << "<@GLOBAL>ipkcache_hits:" << numHits_ << "</@GLOBAL>" << endl;
	cout << "<@GLOBAL>ipkcache_misses:" << numMisses_ << "</@GLOBAL>" << endl;
  }

  if (tagChecks > 0) {
	cout <<
	  "Instruction packet cache:" << "\n" <<
	  "  Reads:    " << numReads_ << "\n" <<
	  "  Writes:   " << numWrites_ << "\n" <<
	  "  Tag checks: " << tagChecks << "\n" <<
	  "    Hits:   " << numHits_ << "\t(" << percentage(numHits_,tagChecks) << ")\n" <<
	  "    Misses: " << numMisses_ << "\t(" << percentage(numMisses_,tagChecks) << ")\n";
  }
}

void IPKCache::dumpEventCounts(std::ostream& os) {
  // TODO: record direct-mapped/fully-associative/etc.
  os << "<ipkcache entries=\"" << IPK_CACHE_SIZE << "\">\n"
     << xmlNode("instances", NUM_CORES) << "\n"
     << xmlNode("active", dataActive_) << "\n"
     << xmlNode("read", numReads_) << "\n"
     << xmlNode("write", numWrites_) << "\n"
     << xmlEnd("ipkcache") << "\n";

  os << "<ipkcachetags entries=\"" << IPK_CACHE_TAGS << "\">\n"
     << xmlNode("instances", NUM_CORES) << "\n"
     << xmlNode("active", tagsActive_) << "\n"
     << xmlNode("hd", tagHD_) << "\n"
     << xmlNode("write", tagWrites_) << "\n"
     << xmlNode("read", numTagChecks()) << "\n"
     << xmlEnd("ipkcachetags") << "\n";
}
