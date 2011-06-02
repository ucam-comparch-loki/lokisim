/*
 * IPKCache.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include "IPKCache.h"

#include "../../Datatype/ComponentID.h"

unsigned long long IPKCache::numHits_ = 0;
unsigned long long IPKCache::numMisses_ = 0;
unsigned long long IPKCache::numReads_ = 0;
unsigned long long IPKCache::numWrites_ = 0;

void IPKCache::tagCheck(const ComponentID& core, bool hit) {
  if(hit) numHits_++;
  else numMisses_++;
}

void IPKCache::read(const ComponentID& core) {
  numReads_++;
}

void IPKCache::write(const ComponentID& core) {
  numWrites_++;
}

unsigned long long IPKCache::numTagChecks() {return numHits_ + numMisses_;}
unsigned long long IPKCache::numHits()      {return numHits_;}
unsigned long long IPKCache::numMisses()    {return numMisses_;}
unsigned long long IPKCache::numReads()     {return numReads_;}
unsigned long long IPKCache::numWrites()    {return numWrites_;}

void IPKCache::printStats() {

	unsigned long long tagChecks = numTagChecks();

  if (BATCH_MODE) {
	cout << "<@GLOBAL>ipkcache_reads:" << numReads_ << "</@GLOBAL>" << endl;
	cout << "<@GLOBAL>ipkcache_writes:" << numWrites_ << "</@GLOBAL>" << endl;
	cout << "<@GLOBAL>ipkcache_tag_checks:" << tagChecks << "</@GLOBAL>" << endl;
	cout << "<@GLOBAL>ipkcache_hits:" << numHits_ << "</@GLOBAL>" << endl;
	cout << "<@GLOBAL>ipkcache_misses:" << numMisses_ << "</@GLOBAL>" << endl;
  }

  if(tagChecks > 0) {
	cout <<
	  "Instruction packet cache:" << "\n" <<
	  "  Reads:    " << numReads_ << "\n" <<
	  "  Writes:   " << numWrites_ << "\n" <<
	  "  Tag checks: " << tagChecks << "\n" <<
	  "    Hits:   " << numHits_ << "\t(" << percentage(numHits_,tagChecks) << ")\n" <<
	  "    Misses: " << numMisses_ << "\t(" << percentage(numMisses_,tagChecks) << ")\n";
  }
}
