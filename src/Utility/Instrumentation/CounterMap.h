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

  void increment(const T& event) {
    counters[event] = counters[event] + 1;
    totalEvents++;
  }

  const int getCount(const T& event) {
    return counters[event];
  }

  const int operator[](const T& event) {
    return getCount(event);
  }

  void setCount(const T& event, int count) {
    totalEvents += (count - counters[event]);
    counters[event] = count;
  }

  const int numEvents() const {
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
