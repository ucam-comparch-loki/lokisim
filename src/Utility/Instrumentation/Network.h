/*
 * Network.h
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "InstrumentationBase.h"
#include "CounterMap.h"

class Network: public InstrumentationBase {

public:

  static void traffic(int startID, int endID, double distance);
  static void printStats();

private:
  static CounterMap<int> commDistances;
  static CounterMap<int> producers;
  static CounterMap<int> consumers;
  static int maxDistance;
  static double totalDistance;

};

#endif /* NETWORK_H_ */
