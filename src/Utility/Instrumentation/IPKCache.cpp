/*
 * IPKCache.cpp
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#include "IPKCache.h"

int IPKCache::numHits = 0;
int IPKCache::numMisses = 0;

void IPKCache::cacheHit(bool hit) {
  if(hit) numHits++;
  else numMisses++;
}

void IPKCache::printStats() {

  int tagChecks = numHits + numMisses;

  if(tagChecks > 0) {
    cout <<
      "Cache:" << endl <<
      "  Tag checks: " << tagChecks << endl <<
      "    Hits:   " << numHits << "\t(" << asPercentage(numHits,tagChecks) << ")" << endl <<
      "    Misses: " << numMisses << "\t(" << asPercentage(numMisses,tagChecks) << ")" << endl;
  }
}
