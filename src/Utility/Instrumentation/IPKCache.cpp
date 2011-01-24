/*
 * IPKCache.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include "IPKCache.h"

int IPKCache::numHits = 0;
int IPKCache::numMisses = 0;
int IPKCache::numReads = 0;
int IPKCache::numWrites = 0;

void IPKCache::tagCheck(ComponentID core, bool hit) {
  if(hit) numHits++;
  else numMisses++;
}

void IPKCache::read(ComponentID core) {
  numReads++;
}

void IPKCache::write(ComponentID core) {
  numWrites++;
}

void IPKCache::printStats() {

  int tagChecks = numHits + numMisses;

  if(tagChecks > 0) {
    cout <<
      "Instruction packet cache:" << "\n" <<
      "  Reads:    " << numReads << "\n" <<
      "  Writes:   " << numWrites << "\n" <<
      "  Tag checks: " << tagChecks << "\n" <<
      "    Hits:   " << numHits << "\t(" << asPercentage(numHits,tagChecks) << ")\n" <<
      "    Misses: " << numMisses << "\t(" << asPercentage(numMisses,tagChecks) << ")\n";
  }
}
