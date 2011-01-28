/*
 * IPKCache.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include "IPKCache.h"

int IPKCache::numHits_ = 0;
int IPKCache::numMisses_ = 0;
int IPKCache::numReads_ = 0;
int IPKCache::numWrites_ = 0;

void IPKCache::tagCheck(ComponentID core, bool hit) {
  if(hit) numHits_++;
  else numMisses_++;
}

void IPKCache::read(ComponentID core) {
  numReads_++;
}

void IPKCache::write(ComponentID core) {
  numWrites_++;
}

int IPKCache::numTagChecks() {return numHits_ + numMisses_;}
int IPKCache::numHits()      {return numHits_;}
int IPKCache::numMisses()    {return numMisses_;}
int IPKCache::numReads()     {return numReads_;}
int IPKCache::numWrites()    {return numWrites_;}

void IPKCache::printStats() {

  int tagChecks = numTagChecks();

  if(tagChecks > 0) {
    cout <<
      "Instruction packet cache:" << "\n" <<
      "  Reads:    " << numReads_ << "\n" <<
      "  Writes:   " << numWrites_ << "\n" <<
      "  Tag checks: " << tagChecks << "\n" <<
      "    Hits:   " << numHits_ << "\t(" << asPercentage(numHits_,tagChecks) << ")\n" <<
      "    Misses: " << numMisses_ << "\t(" << asPercentage(numMisses_,tagChecks) << ")\n";
  }
}
