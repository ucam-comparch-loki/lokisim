/*
 * IPKCache.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include "IPKCache.h"

#include "../../Datatype/ComponentID.h"

int IPKCache::numHits_ = 0;
int IPKCache::numMisses_ = 0;
int IPKCache::numReads_ = 0;
int IPKCache::numWrites_ = 0;

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
      "    Hits:   " << numHits_ << "\t(" << percentage(numHits_,tagChecks) << ")\n" <<
      "    Misses: " << numMisses_ << "\t(" << percentage(numMisses_,tagChecks) << ")\n";
  }
}
