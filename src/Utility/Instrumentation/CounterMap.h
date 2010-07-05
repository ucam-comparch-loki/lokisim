/*
 * CounterMap.h
 *
 * A mapping between elements of a particular type, and integers. It is used
 * to count occurrences of various events.
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#ifndef COUNTERMAP_H_
#define COUNTERMAP_H_

#include <map>

template<class T>
class CounterMap {

public:

  void increment(T& event) {
    counters[event] = counters[event] + 1;
    totalEvents++;
  }

  int getCount(T& event) {
    return counters[event];
  }

  int operator[](T& event) {
    return getCount(event);
  }

  void setCount(T& event, int count) {
    // Note: this will probably invalidate the totalEvents count
    counters[event] = count;
  }

  int numEvents() {
    return totalEvents;
  }

  CounterMap() {

  }

  virtual ~CounterMap() {

  }

private:
  std::map<T,int> counters;
  int totalEvents;

};

#endif /* COUNTERMAP_H_ */
